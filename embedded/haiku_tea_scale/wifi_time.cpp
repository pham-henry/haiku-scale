#include "wifi_time.h"

#include <WiFi.h>
#include "time.h"

/*
  wifi_time.cpp

  Purpose:
  This file handles network connectivity and time synchronization for the ESP32.

  Responsibilities:
  - Connect the ESP32 to Wi-Fi
  - Reconnect if Wi-Fi is lost during runtime
  - Synchronize the device clock using NTP
  - Provide UTC timestamps in ISO 8601 format for backend data uploads

  Why it matters:
  The system needs reliable internet access to send measurements to the backend,
  and accurate timestamps so the backend can analyze tea usage over time.

  // THINK about how much storage can be saved, incase wifi is out, whats the most amount of data i can store and send back to my backend service
*/

void connectWiFi(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void maintainWiFi(const char* ssid, const char* password) {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.println("WiFi disconnected. Reconnecting...");
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Reconnected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Reconnect attempt timed out.");
  }
}

void initTime(const char* ntpServer, long gmtOffsetSec, int daylightOffsetSec) {
  configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);

  Serial.print("Syncing time");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println("Time synced.");
}

String getIsoTimestampUtc() {
  time_t now;
  time(&now);

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);

  char buf[30];
  strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buf);
}