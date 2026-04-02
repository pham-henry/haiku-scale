#pragma once

#include "measurement.h"

void bufferInit();
bool bufferPush(const Measurement& m);
bool bufferPeek(Measurement& m);
bool bufferPop();
bool bufferIsEmpty();
int bufferCount();