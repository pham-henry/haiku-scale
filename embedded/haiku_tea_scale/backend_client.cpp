#include "backend_client.h"

#include <WiFi.h>
#include <HTTPClient.h>

/*
  backend_client.cpp

  Purpose:
  This file handles communication between the ESP32 and the backend server.

  Responsibilities:
  - Build the JSON payload containing weight and timestamp data
  - Send HTTP POST requests to the backend API
  - Report success or failure through the serial monitor

  Why it matters:
  The ESP32’s job is not just to read weight locally, but to stream measurement
  data to the backend where storage, analytics, and prediction logic happen.
*/

void postMeasurement(const char* serverUrl, float weight_g, const String& timestamp) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping POST");
    return;
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = "{";
  payload += "\"timestamp\":\"" + timestamp + "\",";
  payload += "\"weight_g\":" + String(weight_g, 1);
  payload += "}";

  int httpResponseCode = http.POST(payload);

  Serial.print("POST ");
  Serial.print(serverUrl);
  Serial.print(" -> ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response:");
    Serial.println(response);
  } else {
    Serial.print("POST failed, error: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}