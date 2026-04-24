#pragma once

#include "measurement.h"

struct ReportDecision {
  bool shouldSend;
  ReportReason reason;
};

void initReportPolicy();
ReportDecision evaluateReportPolicy(const Measurement& m, bool stateChanged);
void markReportSuccess(const Measurement& m);
float getLastReportedStableWeight();
