/*
  scale_sensor.cpp

  Purpose:
  Read the HX711 and turn one-sided supported load into robust grams.

  It exists as the firmware edge signal layer: raw HX711 samples are median
  filtered, spike rejected, averaged, and scored for stability before higher
  layers decide state or reporting. It deliberately does not auto-tare on boot.
*/

#include "scale_sensor.h"

#include <Arduino.h>
#include "HX711.h"
#include "config.h"

static HX711 scale;
static float avgBuffer[MOVING_AVG_WINDOW] = {0};
static float medianBuffer[MEDIAN_WINDOW] = {0};
static int avgIndex = 0;
static int medianIndex = 0;
static bool avgFilled = false;
static bool medianFilled = false;
static float lastWeight = 0.0f;
static float calibrationFactor = DEFAULT_CALIBRATION_FACTOR;
static long zeroOffsetRaw = DEFAULT_ZERO_OFFSET_RAW;

static int sampleCount(bool filled, int index, int capacity) {
  return filled ? capacity : index;
}

static float medianOfRecent() {
  int count = sampleCount(medianFilled, medianIndex, MEDIAN_WINDOW);
  if (count <= 0) return lastWeight;

  float values[MEDIAN_WINDOW];
  for (int i = 0; i < count; i++) values[i] = medianBuffer[i];

  for (int i = 1; i < count; i++) {
    float key = values[i];
    int j = i - 1;
    while (j >= 0 && values[j] > key) {
      values[j + 1] = values[j];
      j--;
    }
    values[j + 1] = key;
  }

  if (count % 2 == 1) return values[count / 2];
  return (values[(count / 2) - 1] + values[count / 2]) * 0.5f;
}

static float varianceOfAverageWindow(float mean) {
  int count = sampleCount(avgFilled, avgIndex, MOVING_AVG_WINDOW);
  if (count <= 1) return 0.0f;

  float sum = 0.0f;
  for (int i = 0; i < count; i++) {
    float d = avgBuffer[i] - mean;
    sum += d * d;
  }
  return sum / count;
}

void initScale(int doutPin, int sckPin) {
  scale.begin(doutPin, sckPin);
  applyScaleCalibration(calibrationFactor, zeroOffsetRaw);

  delay(500);
  Serial.println("Scale initialized. No auto-tare performed.");
}

bool scaleSignalReady() {
  return scale.is_ready();
}

ScaleReading readScale() {
  ScaleReading reading;
  reading.raw = 0;
  reading.median_g = lastWeight;
  reading.filtered_g = lastWeight;
  reading.variance_g2 = 0.0f;
  reading.signal_ok = scale.is_ready();
  reading.stable = false;
  reading.outlier_rejected = false;

  if (!scale.is_ready()) {
    return reading;
  }

  long raw = scale.read();
  float w = (static_cast<float>(raw - zeroOffsetRaw)) / calibrationFactor;

  if (isnan(w) || isinf(w)) {
    reading.signal_ok = false;
    return reading;
  }

  if (fabs(w) < ZERO_CLAMP_THRESHOLD_G) {
    w = 0.0f;
  }

  if (fabs(w - lastWeight) > IMPOSSIBLE_SPIKE_DELTA_G) {
    reading.outlier_rejected = true;
    w = lastWeight;
  }

  medianBuffer[medianIndex] = w;
  medianIndex = (medianIndex + 1) % MEDIAN_WINDOW;
  if (medianIndex == 0) medianFilled = true;

  float median = medianOfRecent();
  if (fabs(w - median) > OUTLIER_REJECT_DELTA_G) {
    reading.outlier_rejected = true;
    w = median;
  }

  avgBuffer[avgIndex] = w;
  avgIndex = (avgIndex + 1) % MOVING_AVG_WINDOW;
  if (avgIndex == 0) avgFilled = true;

  int count = sampleCount(avgFilled, avgIndex, MOVING_AVG_WINDOW);
  if (count <= 0) {
    lastWeight = w;
    reading.raw = raw;
    reading.median_g = median;
    reading.filtered_g = w;
    return reading;
  }

  float sum = 0.0f;
  for (int i = 0; i < count; i++) {
    sum += avgBuffer[i];
  }

  lastWeight = sum / count;

  if (fabs(lastWeight) < ZERO_CLAMP_THRESHOLD_G) {
    lastWeight = 0.0f;
  }

  reading.raw = raw;
  reading.median_g = median;
  reading.filtered_g = lastWeight;
  reading.variance_g2 = varianceOfAverageWindow(lastWeight);
  reading.signal_ok = reading.variance_g2 < SIGNAL_FAULT_VARIANCE_G2;
  reading.stable = reading.variance_g2 <= STABLE_VARIANCE_G2;
  return reading;
}

float getLastWeight() {
  return lastWeight;
}

long getCurrentRawOffset() {
  return zeroOffsetRaw;
}

void applyScaleCalibration(float factor, long offsetRaw) {
  if (fabs(factor) < 0.0001f) {
    Serial.println("Invalid calibration factor in storage; using default.");
    factor = DEFAULT_CALIBRATION_FACTOR;
  }

  calibrationFactor = factor;
  zeroOffsetRaw = offsetRaw;
  scale.set_scale(calibrationFactor);
  scale.set_offset(zeroOffsetRaw);
}

bool captureSafeZeroOffset() {
  if (!scale.is_ready() || fabs(lastWeight) > SAFE_TARE_MAX_WEIGHT_G) {
    return false;
  }

  zeroOffsetRaw = scale.read_average(10);
  applyScaleCalibration(calibrationFactor, zeroOffsetRaw);
  lastWeight = 0.0f;
  return true;
}
