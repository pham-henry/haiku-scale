/*
  backend_client.cpp

  POST compact device events from the ESP32 to the local backend.

  It exists as the network boundary: callers get a simple success/failure
  result so firmware can continue measuring and buffer locally during Wi-Fi or
  backend outages.
*/

#include "backend_client.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"

bool postMeasurement(const char* serverUrl, const char* deviceId, const Measurement& m, bool buffered, ReportReason reason) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping POST");
    return false;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"device_id\":\"" + String(deviceId) + "\",";
  payload += "\"unix_time\":" + String(m.unixTime) + ",";
  payload += "\"uptime_ms\":" + String(m.uptimeMs) + ",";
  payload += "\"firmware_version\":\"" + String(FIRMWARE_VERSION) + "\",";
  payload += "\"raw\":" + String(m.raw) + ",";
  payload += "\"median_g\":" + String(m.median_g, 1) + ",";
  payload += "\"weight_g\":" + String(m.weight_g, 1) + ",";
  payload += "\"variance_g2\":" + String(m.variance_g2, 2) + ",";
  payload += "\"state\":\"" + String(deviceStateToString(m.state)) + "\",";
  payload += "\"signal_ok\":" + String(m.signal_ok ? "true" : "false") + ",";
  payload += "\"stable\":" + String(m.stable ? "true" : "false") + ",";
  payload += "\"outlier_rejected\":" + String(m.outlier_rejected ? "true" : "false") + ",";
  payload += "\"buffered\":" + String(buffered ? "true" : "false") + ",";
  payload += "\"report_reason\":\"" + String(reportReasonToString(reason)) + "\"";
  payload += "}";

  int code = http.POST(payload);

  Serial.print("POST -> ");
  Serial.println(code);

  if (code > 0 && code < 300) {
    String response = http.getString();
    Serial.println("POST success");
    if (response.length() > 0) {
      Serial.println(response);
    }
    http.end();
    return true;
  }

  Serial.print("POST failed: ");
  Serial.println(code);
  http.end();
  return false;
}
