#include <Arduino.h>

#include "config.h"
#include "secrets.h"
#include "measurement.h"
#include "wifi_time.h"
#include "backend_client.h"
#include "scale_sensor.h"
#include "buffer_store.h"
#include "state_detector.h"
#include "calibration_manager.h"
#include "report_policy.h"

unsigned long lastSampleMs = 0;
unsigned long lastBufferedRecordMs = 0;

void tryFlushBuffer() {
  if (bufferIsEmpty()) return;

  Serial.print("Flushing buffer, count = ");
  Serial.println(bufferCount());

  int sent = 0;
  BufferedMeasurement item;
  while (!bufferIsEmpty() && sent < FLUSH_BATCH_LIMIT) {
    if (!bufferPeek(item)) break;

    bool ok = postMeasurement(SERVER_URL, DEVICE_ID, item.measurement, true, REPORT_BUFFER_FLUSH);
    if (!ok) {
      Serial.println("Flush stopped; backend still unavailable.");
      return;
    }
    bufferPop();
    sent++;
    delay(50);
  }

  Serial.print("Buffer flush pass complete. Remaining = ");
  Serial.println(bufferCount());
}

Measurement buildMeasurement(const ScaleReading& reading, DeviceState state) {
  Measurement m;
  m.unixTime = isTimeSynced() ? getUnixTimeUtc() : 0;
  m.uptimeMs = millis();
  m.raw = reading.raw;
  m.median_g = reading.median_g;
  m.weight_g = reading.filtered_g;
  m.variance_g2 = reading.variance_g2;
  m.stable = reading.stable && isWeightStable();
  m.state = state;
  m.signal_ok = reading.signal_ok;
  m.outlier_rejected = reading.outlier_rejected;
  return m;
}

void recordOfflineSnapshot(const Measurement& m, ReportReason reason) {
  unsigned long now = millis();

  if (now - lastBufferedRecordMs < OFFLINE_BUFFER_INTERVAL_MS) return;

  lastBufferedRecordMs = now;
  bufferPush(m, reason);

  Serial.print("Buffered sample. Count = ");
  Serial.println(bufferCount());
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Booting Haiku Tea Scale...");

  bufferInit();
  initStateDetector();
  initReportPolicy();

  connectWiFi(WIFI_SSID, WIFI_PASSWORD);
  initTime(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);

  initScale(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  initCalibrationManager();

  Serial.println("System ready.");
  Serial.println("IMPORTANT: no auto-tare is performed on boot.");
  Serial.println("Readings are supported-side grams from the slanted instrumented side.");
}

void loop() {
  unsigned long now = millis();

  maintainWiFi(WIFI_SSID, WIFI_PASSWORD);
  retryTimeSyncIfNeeded(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);

  if (WiFi.status() == WL_CONNECTED) {
    tryFlushBuffer();
  }

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;

    ScaleReading reading = readScale();
    DeviceState state = updateDeviceState(reading);
    Measurement m = buildMeasurement(reading, state);

    maybeApplyEmptyDriftCorrection(m.weight_g, state == STATE_EMPTY_STAND && m.stable && m.signal_ok);

    Serial.print("Weight(g): ");
    Serial.print(m.weight_g, 1);
    Serial.print(" | Raw: ");
    Serial.print(m.raw);
    Serial.print(" | Var: ");
    Serial.print(m.variance_g2, 1);
    Serial.print(" | State: ");
    Serial.print(deviceStateToString(state));
    Serial.print(" | Stable: ");
    Serial.print(m.stable ? "Y" : "N");
    Serial.print(" | Signal: ");
    Serial.print(m.signal_ok ? "OK" : "FAULT");
    Serial.print(" | WiFi: ");
    Serial.print(WiFi.status() == WL_CONNECTED ? "OK" : "DOWN");
    Serial.print(" | Buffer: ");
    Serial.println(bufferCount());

    ReportDecision decision = evaluateReportPolicy(m, didStateChange());

    if (decision.shouldSend) {
      Serial.print("Report decision: ");
      Serial.println(reportReasonToString(decision.reason));

      bool ok = postMeasurement(SERVER_URL, DEVICE_ID, m, false, decision.reason);

      if (ok) {
        markReportSuccess(m);
      } else {
        recordOfflineSnapshot(m, decision.reason);
      }
    } else if (WiFi.status() != WL_CONNECTED) {
      recordOfflineSnapshot(m, REPORT_HEARTBEAT);
    }
  }
}
