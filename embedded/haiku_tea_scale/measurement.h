#pragma once

#include <Arduino.h>

enum DeviceState : uint8_t {
  STATE_EMPTY_STAND = 0,
  STATE_CONTAINER_PRESENT_STABLE = 1,
  STATE_WEIGHT_CHANGING = 2,
  STATE_CONTAINER_REMOVED = 3,
  STATE_SENSOR_FAULT = 4,
  STATE_REFILL_DETECTED = 5,
  STATE_POURING = 6,
  STATE_TEMPORARY_LIFT = 7,
  STATE_UNSTABLE = 8
};

enum ReportReason : uint8_t {
  REPORT_BOOT = 0,
  REPORT_STATE_CHANGE = 1,
  REPORT_STABLE_WEIGHT_DELTA = 2,
  REPORT_HEARTBEAT = 3,
  REPORT_CRITICAL_EVENT = 4,
  REPORT_BUFFER_FLUSH = 5
};

struct Measurement {
  uint32_t unixTime;
  uint32_t uptimeMs;
  long raw;
  float median_g;
  float weight_g;
  float variance_g2;
  bool stable;
  DeviceState state;
  bool signal_ok;
  bool outlier_rejected;
};

inline const char* deviceStateToString(DeviceState state) {
  switch (state) {
    case STATE_EMPTY_STAND: return "empty_stand";
    case STATE_CONTAINER_PRESENT_STABLE: return "container_present_stable";
    case STATE_WEIGHT_CHANGING: return "weight_changing";
    case STATE_CONTAINER_REMOVED: return "container_removed";
    case STATE_SENSOR_FAULT: return "sensor_fault";
    case STATE_REFILL_DETECTED: return "refill_detected";
    case STATE_POURING: return "pouring";
    case STATE_TEMPORARY_LIFT: return "temporary_lift";
    case STATE_UNSTABLE: return "unstable";
    default: return "unknown";
  }
}

inline const char* reportReasonToString(ReportReason reason) {
  switch (reason) {
    case REPORT_BOOT: return "boot";
    case REPORT_STATE_CHANGE: return "state_change";
    case REPORT_STABLE_WEIGHT_DELTA: return "stable_weight_delta";
    case REPORT_HEARTBEAT: return "heartbeat";
    case REPORT_CRITICAL_EVENT: return "critical_event";
    case REPORT_BUFFER_FLUSH: return "buffer_flush";
    default: return "unknown";
  }
}
