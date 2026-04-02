#pragma once

#include <Arduino.h>
#include "measurement.h"

bool postMeasurement(const char* serverUrl, const char* deviceId, const Measurement& m, bool buffered);