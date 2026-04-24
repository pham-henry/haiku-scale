/*
  state_detector.cpp

  Convert robust supported-side load readings into physical stand states.

  It exists between sensing and networking: the backend gets context such as
  removal, refill, pouring, instability, and fault instead of having to infer
  every cafe interaction from raw grams.
*/

#include "state_detector.h"

#include <Arduino.h>
#include "config.h"
#include "scale_sensor.h"

static DeviceState currentState = STATE_EMPTY_STAND;
static DeviceState previousState = STATE_EMPTY_STAND;
static bool stateChanged = false;

static float lastWeight = 0.0f;
static float lastStableWeight = 0.0f;
static unsigned long lastMotionMs = 0;
static unsigned long removedSinceMs = 0;
static bool stableNow = false;
static bool hadContainer = false;

void initStateDetector() {
  currentState = STATE_EMPTY_STAND;
  previousState = STATE_EMPTY_STAND;
  stateChanged = false;
  lastWeight = 0.0f;
  lastStableWeight = 0.0f;
  lastMotionMs = millis();
  removedSinceMs = 0;
  stableNow = false;
  hadContainer = false;
}

DeviceState updateDeviceState(const ScaleReading& reading) {
  unsigned long now = millis();
  stateChanged = false;
  float weight_g = reading.filtered_g;

  if (!reading.signal_ok) {
    previousState = currentState;
    currentState = STATE_SENSOR_FAULT;
    stateChanged = (currentState != previousState);
    return currentState;
  }

  float delta = fabs(weight_g - lastWeight);

  if (delta > STABLE_DELTA_G || !reading.stable) {
    lastMotionMs = now;
    stableNow = false;
  } else if (now - lastMotionMs >= STABLE_TIME_MS) {
    stableNow = true;
  }

  DeviceState nextState = currentState;

  if (weight_g <= EMPTY_STAND_THRESHOLD_G) {
    if (hadContainer) {
      nextState = STATE_CONTAINER_REMOVED;
      hadContainer = false;
      removedSinceMs = now;
    } else {
      nextState = STATE_EMPTY_STAND;
    }
  } else if (!stableNow) {
    if (removedSinceMs > 0 && now - removedSinceMs <= TEMPORARY_LIFT_WINDOW_MS) {
      nextState = STATE_TEMPORARY_LIFT;
    } else if (reading.variance_g2 > STABLE_VARIANCE_G2 * 8.0f) {
      nextState = STATE_UNSTABLE;
    } else if (lastStableWeight > CONTAINER_PRESENT_THRESHOLD_G && weight_g < lastStableWeight - REPORT_DELTA_G) {
      nextState = STATE_POURING;
    } else if (lastStableWeight > CONTAINER_PRESENT_THRESHOLD_G && weight_g > lastStableWeight + MAJOR_REFILL_DELTA_G) {
      nextState = STATE_REFILL_DETECTED;
    } else {
      nextState = STATE_WEIGHT_CHANGING;
    }

    if (weight_g >= CONTAINER_PRESENT_THRESHOLD_G) {
      hadContainer = true;
    }
  } else {
    if (weight_g >= CONTAINER_PRESENT_THRESHOLD_G) {
      hadContainer = true;
      if (lastStableWeight > CONTAINER_PRESENT_THRESHOLD_G && weight_g > lastStableWeight + MAJOR_REFILL_DELTA_G) {
        nextState = STATE_REFILL_DETECTED;
      } else {
        nextState = STATE_CONTAINER_PRESENT_STABLE;
      }
      lastStableWeight = weight_g;
      removedSinceMs = 0;
    } else {
      nextState = STATE_EMPTY_STAND;
      lastStableWeight = weight_g;
    }
  }

  previousState = currentState;
  currentState = nextState;
  stateChanged = (currentState != previousState);

  lastWeight = weight_g;
  return currentState;
}

DeviceState getCurrentState() {
  return currentState;
}

bool didStateChange() {
  return stateChanged;
}

bool isWeightStable() {
  return stableNow;
}

float getLastStableWeight() {
  return lastStableWeight;
}
