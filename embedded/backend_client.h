#pragma once

#include <Arduino.h>

void postMeasurement(const char* serverUrl, float weight_g, const String& timestamp);