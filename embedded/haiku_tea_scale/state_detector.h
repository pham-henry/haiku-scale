#pragma once

#include "measurement.h"

void initStateDetector();
DeviceState updateDeviceState(float weight_g, bool signal_ok);
DeviceState getCurrentState();
bool didStateChange();
bool isWeightStable();