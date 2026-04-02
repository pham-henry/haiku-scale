#pragma once

void initScale(int doutPin, int sckPin, float calibrationFactor);
bool scaleSignalReady();
float readFilteredWeight();
float getLastWeight();