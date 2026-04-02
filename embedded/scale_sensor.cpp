#include "scale_sensor.h"

#include <Arduino.h>
#include "HX711.h"
#include "config.h"

/*
  scale_sensor.cpp

  Purpose:
  This file manages the load cell and HX711 amplifier used to measure tea container weight.

  Responsibilities:
  - Initialize the HX711 and apply the calibration factor
  - Tare the scale at startup
  - Read weight data from the sensor
  - Filter noisy readings using a moving average
  - Return a stable weight value for the rest of the system

  Why it matters:
  This is the sensing layer of the project. Accurate and stable weight readings
  are critical because the backend depends on this data to estimate tea depletion
  and determine when workers should be alerted.
*/

static HX711 scale;

static float weightBuffer[MOVING_AVG_WINDOW] = {0};
static int bufferIndex = 0;
static bool bufferFilled = false;

static float lastWeight = 0.0f;

void initScale(int doutPin, int sckPin, float calibrationFactor) {
  scale.begin(doutPin, sckPin);
  scale.set_scale(calibrationFactor);

  delay(500);   // allow sensor to settle
  scale.tare(); // assumes empty stand at startup

  Serial.println("Scale initialized and tared.");
}

float readFilteredWeight() {
  if (!scale.is_ready()) {
    return lastWeight;
  }

  float w = scale.get_units(1);

  if (isnan(w) || isinf(w)) {
    return lastWeight;
  }

  weightBuffer[bufferIndex] = w;
  bufferIndex = (bufferIndex + 1) % MOVING_AVG_WINDOW;

  if (bufferIndex == 0) {
    bufferFilled = true;
  }

  int count = bufferFilled ? MOVING_AVG_WINDOW : bufferIndex;

  if (count <= 0) {
    lastWeight = w;
    return w;
  }

  float sum = 0.0f;
  for (int i = 0; i < count; i++) {
    sum += weightBuffer[i];
  }

  float avg = sum / count;

  if (fabs(avg) < ZERO_CLAMP_THRESHOLD_G) {
    avg = 0.0f;
  }

  lastWeight = avg;
  return avg;
}

float getLastWeight() {
  return lastWeight;
}