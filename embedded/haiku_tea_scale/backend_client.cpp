/*
  backend_client.cpp

  Purpose:
  This file sends measurement data from the ESP32 to the backend API.

  Responsibilities:
  - Format measurement data as JSON
  - POST data to the backend
  - Return success/failure so the caller can decide whether to buffer or retry

  Why it matters:
  The ESP32 is the sensing edge device. This file connects that device to the
  backend system where storage, analytics, and prediction logic happen.
*/

#include "backend_client.h"

#include <WiFi.h>
#include <HTTPClient.h>

bool postMeasurement(const char* serverUrl, const char* deviceId, const Measurement& m, bool buffered) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping POST");
    return false;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"device_id\":\"" + String(deviceId) + "\",";
  payload += "\"timestamp\":\"" + String((m.unixTime > 0) ? "" : "") + "\",";
  payload += "\"unix_time\":" + String(m.unixTime) + ",";
  payload += "\"weight_g\":" + String(m.weight_g, 1) + ",";
  payload += "\"state\":\"" + String(deviceStateToString(m.state)) + "\",";
  payload += "\"signal_ok\":" + String(m.signal_ok ? "true" : "false") + ",";
  payload += "\"buffered\":" + String(buffered ? "true" : "false");
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