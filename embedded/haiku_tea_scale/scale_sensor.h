#pragma once

#include "measurement.h"

struct ScaleReading {
  long raw;
  float median_g;
  float filtered_g;
  float variance_g2;
  bool signal_ok;
  bool stable;
  bool outlier_rejected;
};

void initScale(int doutPin, int sckPin);
bool scaleSignalReady();
ScaleReading readScale();
float getLastWeight();
long getCurrentRawOffset();
void applyScaleCalibration(float calibrationFactor, long zeroOffsetRaw);
bool captureSafeZeroOffset();
