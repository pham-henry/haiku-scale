/*
  buffer_store.cpp

  Provide a compact in-memory outage buffer for device events.

  It exists so sensing continues while Wi-Fi or the backend is down. The buffer
  stores structs, flushes oldest-first, and intentionally overwrites oldest data
  after the configured 30-minute MVP window.
*/

#include "buffer_store.h"
#include "config.h"

static BufferedMeasurement ringBuffer[BUFFER_SIZE];
static int head = 0;
static int tail = 0;
static int count = 0;

void bufferInit() {
  head = 0;
  tail = 0;
  count = 0;
}

bool bufferPush(const Measurement& m, ReportReason reason) {
  if (count == BUFFER_SIZE) {
    // overwrite oldest
    tail = (tail + 1) % BUFFER_SIZE;
    count--;
  }

  ringBuffer[head].measurement = m;
  ringBuffer[head].reason = reason;
  head = (head + 1) % BUFFER_SIZE;
  count++;
  return true;
}

bool bufferPeek(BufferedMeasurement& item) {
  if (count == 0) return false;
  item = ringBuffer[tail];
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
