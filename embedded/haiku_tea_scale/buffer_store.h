#pragma once

#include "measurement.h"

struct BufferedMeasurement {
  Measurement measurement;
  ReportReason reason;
};

void bufferInit();
bool bufferPush(const Measurement& m, ReportReason reason);
bool bufferPeek(BufferedMeasurement& item);
bool bufferPop();
bool bufferIsEmpty();
int bufferCount();
