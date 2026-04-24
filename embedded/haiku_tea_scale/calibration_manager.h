#pragma once

struct CalibrationSettings {
  float calibrationFactor;
  long zeroOffsetRaw;
  float point1KnownG;
  long point1Raw;
  float point2KnownG;
  long point2Raw;
  bool hasTwoPointCalibration;
};

void initCalibrationManager();
CalibrationSettings getCalibrationSettings();
void saveCalibrationFactor(float calibrationFactor);
void saveZeroOffsetRaw(long zeroOffsetRaw);
void saveTwoPointCalibration(float point1KnownG, long point1Raw, float point2KnownG, long point2Raw);
void maybeApplyEmptyDriftCorrection(float stableWeightG, bool emptyAndStable);
