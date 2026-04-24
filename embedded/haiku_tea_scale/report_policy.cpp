/*
  report_policy.cpp

  Decide when a local measurement is worth sending to the backend.

  This module keeps API traffic low by sending stable meaningful deltas,
  important state transitions, critical fault/removal events, and slow
  heartbeats rather than streaming noisy samples every loop.
*/

#include "report_policy.h"

#include <Arduino.h>
#include "config.h"

static bool hasReportedOnce = false;
static float lastReportedStableWeight = 0.0f;
static unsigned long lastReportMs = 0;
static unsigned long stableSinceMs = 0;
static DeviceState lastReportedState = STATE_EMPTY_STAND;

static bool isCriticalState(DeviceState state) {
  return state == STATE_CONTAINER_REMOVED ||
         state == STATE_SENSOR_FAULT ||
         state == STATE_REFILL_DETECTED;
}

static bool canRateLimitBypass(DeviceState state) {
  return isCriticalState(state);
}

void initReportPolicy() {
  hasReportedOnce = false;
  lastReportedStableWeight = 0.0f;
  lastReportMs = 0;
  stableSinceMs = 0;
  lastReportedState = STATE_EMPTY_STAND;
}

ReportDecision evaluateReportPolicy(const Measurement& m, bool stateChanged) {
  unsigned long now = millis();
  ReportDecision decision = {false, REPORT_HEARTBEAT};

  if (!hasReportedOnce) {
    decision.shouldSend = true;
    decision.reason = REPORT_BOOT;
    return decision;
  }

  if (!canRateLimitBypass(m.state) && now - lastReportMs < MIN_REPORT_INTERVAL_MS) {
    return decision;
  }

  if (isCriticalState(m.state) && m.state != lastReportedState) {
    decision.shouldSend = true;
    decision.reason = REPORT_CRITICAL_EVENT;
    return decision;
  }

  if (stateChanged &&
      (m.state == STATE_EMPTY_STAND ||
       m.state == STATE_CONTAINER_PRESENT_STABLE ||
       m.state == STATE_POURING ||
       m.state == STATE_TEMPORARY_LIFT)) {
    decision.shouldSend = true;
    decision.reason = REPORT_STATE_CHANGE;
    return decision;
  }

  if (m.stable && m.signal_ok &&
      (m.state == STATE_CONTAINER_PRESENT_STABLE ||
       m.state == STATE_EMPTY_STAND ||
       m.state == STATE_REFILL_DETECTED)) {
    if (stableSinceMs == 0) stableSinceMs = now;

    float threshold = REPORT_DELTA_G + REPORT_HYSTERESIS_G;
    if (now - stableSinceMs >= STABLE_REPORT_DWELL_MS &&
        fabs(m.weight_g - lastReportedStableWeight) >= threshold) {
      decision.shouldSend = true;
      decision.reason = REPORT_STABLE_WEIGHT_DELTA;
      return decision;
    }
  } else {
    stableSinceMs = 0;
  }

  if (now - lastReportMs >= HEARTBEAT_INTERVAL_MS) {
    decision.shouldSend = true;
    decision.reason = REPORT_HEARTBEAT;
    return decision;
  }

  return decision;
}

void markReportSuccess(const Measurement& m) {
  hasReportedOnce = true;
  lastReportMs = millis();
  lastReportedState = m.state;

  if (m.stable && m.signal_ok) {
    lastReportedStableWeight = m.weight_g;
    stableSinceMs = 0;
  }
}

float getLastReportedStableWeight() {
  return lastReportedStableWeight;
}
