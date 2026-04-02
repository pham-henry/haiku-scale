/*
  state_detector.cpp

  Purpose:
  This file converts filtered weight readings into simple device states.

  Responsibilities:
  - Detect whether the stand is empty
  - Detect whether a container is present and stable
  - Detect changing/unstable weight
  - Detect container removal
  - Detect sensor fault

  Why it matters:
  Sending only raw weight is not enough. State information helps the backend
  understand what physically happened at the stand.
*/

#include "state_detector.h"

#include <Arduino.h>
#include "config.h"

static DeviceState currentState = STATE_EMPTY_STAND;
static DeviceState previousState = STATE_EMPTY_STAND;
static bool stateChanged = false;

static float lastWeight = 0.0f;
static unsigned long lastMotionMs = 0;
static bool stableNow = false;
static bool hadContainer = false;

void initStateDetector() {
  currentState = STATE_EMPTY_STAND;
  previousState = STATE_EMPTY_STAND;
  stateChanged = false;
  lastWeight = 0.0f;
  lastMotionMs = millis();
  stableNow = false;
  hadContainer = false;
}

DeviceState updateDeviceState(float weight_g, bool signal_ok) {
  unsigned long now = millis();
  stateChanged = false;

  if (!signal_ok) {
    previousState = currentState;
    currentState = STATE_SENSOR_FAULT;
    stateChanged = (currentState != previousState);
    return currentState;
  }

  float delta = fabs(weight_g - lastWeight);

  if (delta > STABLE_DELTA_G) {
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
    } else {
      nextState = STATE_EMPTY_STAND;
    }
  } else if (!stableNow) {
    nextState = STATE_WEIGHT_CHANGING;
    if (weight_g >= CONTAINER_PRESENT_THRESHOLD_G) {
      hadContainer = true;
    }
  } else {
    nextState = STATE_CONTAINER_PRESENT_STABLE;
    if (weight_g >= CONTAINER_PRESENT_THRESHOLD_G) {
      hadContainer = true;
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