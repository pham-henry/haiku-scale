/*
  buffer_store.cpp

  Purpose:
  This file provides an in-memory ring buffer for measurement data when Wi-Fi
  or the backend is unavailable.

  Responsibilities:
  - Store measurements during outages
  - Hold up to 30 minutes of 1 Hz data
  - Flush oldest-first when connectivity returns

  Why it matters:
  This keeps the system resilient during temporary outages without losing recent data.
*/

#include "buffer_store.h"
#include "config.h"

static Measurement ringBuffer[BUFFER_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;

void bufferInit() {
  head = 0;
  tail = 0;
  count = 0;
}

bool bufferPush(const Measurement& m) {
  if (count == BUFFER_SIZE) {
    // overwrite oldest
    tail = (tail + 1) % BUFFER_SIZE;
    count--;
  }

  ringBuffer[head] = m;
  head = (head + 1) % BUFFER_SIZE;
  count++;
  return true;
}

bool bufferPeek(Measurement& m) {
  if (count == 0) return false;
  m = ringBuffer[tail];
  return true;
}

bool bufferPop() {
  if (count == 0) return false;
  tail = (tail + 1) % BUFFER_SIZE;
  count--;
  return true;
}

bool bufferIsEmpty() {
  return count == 0;
}

int bufferCount() {
  return count;
}