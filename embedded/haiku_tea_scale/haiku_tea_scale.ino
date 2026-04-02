#include <Arduino.h>

#include "config.h"
#include "secrets.h"
#include "measurement.h"
#include "wifi_time.h"
#include "backend_client.h"
#include "scale_sensor.h"
#include "buffer_store.h"
#include "state_detector.h"

unsigned long lastSampleMs = 0;
unsigned long lastHeartbeatMs = 0;
unsigned long lastBufferedRecordMs = 0;

float currentWeight = 0.0f;
float lastReportedWeight = 0.0f;
bool hasReportedOnce = false;

void tryFlushBuffer() {
  if (bufferIsEmpty()) return;

  Serial.print("Flushing buffer, count = ");
  Serial.println(bufferCount());

  Measurement m;
  while (!bufferIsEmpty()) {
    if (!bufferPeek(m)) break;

    bool ok = postMeasurement(SERVER_URL, DEVICE_ID, m, true);
    if (!ok) {
      Serial.println("Flush stopped; backend still unavailable.");
      return;
    }
    bufferPop();
    delay(50);
  }

  Serial.println("Buffer flush complete.");
}

Measurement buildMeasurement(float weight, DeviceState state, bool signalOk) {
  Measurement m;
  m.unixTime = isTimeSynced() ? getUnixTimeUtc() : 0;
  m.weight_g = weight;
  m.state = state;
  m.signal_ok = signalOk;
  return m;
}

bool shouldReport(const Measurement& m) {
  unsigned long now = millis();

  if (!hasReportedOnce) return true;

  if (didStateChange()) return true;

  if (fabs(m.weight_g - lastReportedWeight) >= REPORT_DELTA_G) return true;

  if (now - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) return true;

  return false;
}

void recordOfflineSnapshot(const Measurement& m) {
  unsigned long now = millis();

  // keep offline recording to 1 Hz max
  if (now - lastBufferedRecordMs < 1000) return;

  lastBufferedRecordMs = now;
  bufferPush(m);

  Serial.print("Buffered sample. Count = ");
  Serial.println(bufferCount());
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Booting Haiku Tea Scale...");

  bufferInit();
  initStateDetector();

  connectWiFi(WIFI_SSID, WIFI_PASSWORD);
  initTime(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);

  initScale(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN, CALIBRATION_FACTOR);

  Serial.println("System ready.");
  Serial.println("IMPORTANT: no auto-tare is performed on boot.");
}

void loop() {
  unsigned long now = millis();

  maintainWiFi(WIFI_SSID, WIFI_PASSWORD);

  if (WiFi.status() == WL_CONNECTED) {
    tryFlushBuffer();
  }

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;

    bool signalOk = scaleSignalReady();
    currentWeight = readFilteredWeight();
    DeviceState state = updateDeviceState(currentWeight, signalOk);

    Serial.print("Weight(g): ");
    Serial.print(currentWeight, 1);
    Serial.print(" | State: ");
    Serial.print(deviceStateToString(state));
    Serial.print(" | Signal: ");
    Serial.println(signalOk ? "OK" : "FAULT");

    Measurement m = buildMeasurement(currentWeight, state, signalOk);

    if (shouldReport(m)) {
      bool ok = postMeasurement(SERVER_URL, DEVICE_ID, m, false);

      if (ok) {
        lastReportedWeight = m.weight_g;
        lastHeartbeatMs = now;
        hasReportedOnce = true;
      } else {
        recordOfflineSnapshot(m);
      }
    } else if (WiFi.status() != WL_CONNECTED) {
      recordOfflineSnapshot(m);
    }
  }
}