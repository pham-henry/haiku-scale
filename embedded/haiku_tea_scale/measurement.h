#pragma once

#include <Arduino.h>

enum DeviceState : uint8_t {
  STATE_EMPTY_STAND = 0,
  STATE_CONTAINER_PRESENT_STABLE = 1,
  STATE_WEIGHT_CHANGING = 2,
  STATE_CONTAINER_REMOVED = 3,
  STATE_SENSOR_FAULT = 4
};

struct Measurement {
  uint32_t unixTime;
  float weight_g;
  DeviceState state;
  bool signal_ok;
};

inline const char* deviceStateToString(DeviceState state) {
  switch (state) {
    case STATE_EMPTY_STAND: return "empty_stand";
    case STATE_CONTAINER_PRESENT_STABLE: return "container_present_stable";
    case STATE_WEIGHT_CHANGING: return "weight_changing";
    case STATE_CONTAINER_REMOVED: return "container_removed";
    case STATE_SENSOR_FAULT: return "sensor_fault";
    default: return "unknown";
  }
}