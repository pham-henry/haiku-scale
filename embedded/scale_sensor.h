#pragma once

void initScale(int doutPin, int sckPin, float calibrationFactor);

// Returns filtered weight in grams
float readFilteredWeight();

float getLastWeight();