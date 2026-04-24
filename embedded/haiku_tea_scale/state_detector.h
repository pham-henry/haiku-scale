#pragma once

#include "measurement.h"

struct ScaleReading;

void initStateDetector();
DeviceState updateDeviceState(const ScaleReading& reading);
DeviceState getCurrentState();
bool didStateChange();
bool isWeightStable();
float getLastStableWeight();
