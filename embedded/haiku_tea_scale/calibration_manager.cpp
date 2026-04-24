/*
  calibration_manager.cpp

  Persist calibration and safe zero information in ESP32 non-volatile storage.

  This module keeps load-cell calibration out of the main loop, supports a
  future two-point fit, and enforces the project rule that boot must not blindly
  tare because a partially full container may already be on the stand.
*/

#include "calibration_manager.h"

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"
#include "scale_sensor.h"

static Preferences prefs;
static CalibrationSettings settings;
static unsigned long emptyStableSinceMs = 0;

void initCalibrationManager() {
  prefs.begin(CALIBRATION_NAMESPACE, false);

  settings.calibrationFactor = prefs.getFloat("cal", DEFAULT_CALIBRATION_FACTOR);
  settings.zeroOffsetRaw = prefs.getLong("zero", DEFAULT_ZERO_OFFSET_RAW);
  settings.point1KnownG = prefs.getFloat("p1g", 0.0f);
  settings.point1Raw = prefs.getLong("p1raw", 0);
  settings.point2KnownG = prefs.getFloat("p2g", 0.0f);
  settings.point2Raw = prefs.getLong("p2raw", 0);
  settings.hasTwoPointCalibration = prefs.getBool("twoPoint", false);

  applyScaleCalibration(settings.calibrationFactor, settings.zeroOffsetRaw);

  Serial.print("Calibration loaded. factor=");
  Serial.print(settings.calibrationFactor, 4);
  Serial.print(" zero_raw=");
  Serial.println(settings.zeroOffsetRaw);
}

CalibrationSettings getCalibrationSettings() {
  return settings;
}

void saveCalibrationFactor(float calibrationFactor) {
  settings.calibrationFactor = calibrationFactor;
  prefs.putFloat("cal", calibrationFactor);
  applyScaleCalibration(settings.calibrationFactor, settings.zeroOffsetRaw);
}

void saveZeroOffsetRaw(long zeroOffsetRaw) {
  settings.zeroOffsetRaw = zeroOffsetRaw;
  prefs.putLong("zero", zeroOffsetRaw);
  applyScaleCalibration(settings.calibrationFactor, settings.zeroOffsetRaw);
}

void saveTwoPointCalibration(float point1KnownG, long point1Raw, float point2KnownG, long point2Raw) {
  if (fabs(point2KnownG - point1KnownG) < 1.0f || point2Raw == point1Raw) {
    Serial.println("Rejected two-point calibration: points too close.");
    return;
  }

  settings.point1KnownG = point1KnownG;
  settings.point1Raw = point1Raw;
  settings.point2KnownG = point2KnownG;
  settings.point2Raw = point2Raw;
  settings.hasTwoPointCalibration = true;
  settings.calibrationFactor = static_cast<float>(point2Raw - point1Raw) / (point2KnownG - point1KnownG);
  settings.zeroOffsetRaw = point1Raw - static_cast<long>(settings.calibrationFactor * point1KnownG);

  prefs.putFloat("p1g", point1KnownG);
  prefs.putLong("p1raw", point1Raw);
  prefs.putFloat("p2g", point2KnownG);
  prefs.putLong("p2raw", point2Raw);
  prefs.putBool("twoPoint", true);
  prefs.putFloat("cal", settings.calibrationFactor);
  prefs.putLong("zero", settings.zeroOffsetRaw);

  applyScaleCalibration(settings.calibrationFactor, settings.zeroOffsetRaw);
}

void maybeApplyEmptyDriftCorrection(float stableWeightG, bool emptyAndStable) {
  unsigned long now = millis();

  if (!emptyAndStable || fabs(stableWeightG) > SAFE_TARE_MAX_WEIGHT_G) {
    emptyStableSinceMs = 0;
    return;
  }

  if (emptyStableSinceMs == 0) {
    emptyStableSinceMs = now;
    return;
  }

  if (now - emptyStableSinceMs < EMPTY_ZERO_DWELL_MS) {
    return;
  }

  if (captureSafeZeroOffset()) {
    saveZeroOffsetRaw(getCurrentRawOffset());
    Serial.println("Applied safe empty drift correction.");
  }

  emptyStableSinceMs = now;
}
