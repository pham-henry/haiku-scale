/*
  wifi_time.cpp

  Purpose:
  This file manages Wi-Fi connectivity and UTC time synchronization.

  Responsibilities:
  - Connect the ESP32 to Wi-Fi
  - Reconnect automatically if Wi-Fi drops
  - Sync time using NTP
  - Provide Unix time and ISO 8601 UTC timestamps for measurements
*/

#include "wifi_time.h"

#include <WiFi.h>
#include "time.h"

static bool g_timeSynced = false;

void connectWiFi(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");

  unsigned long start = millis();
  const unsigned long timeout = 10000; // 10 seconds

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connection failed. Continuing without WiFi.");
  }
}

void maintainWiFi(const char* ssid, const char* password) {
  if (WiFi.status() == WL_CONNECTED) return;

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
  unsigned long start = millis();

  while (!getLocalTime(&timeinfo) && millis() - start < 15000) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  g_timeSynced = getLocalTime(&timeinfo);
  if (g_timeSynced) {
    Serial.println("Time synced.");
  } else {
    Serial.println("Time sync failed. Will continue and retry later.");
  }
}

bool isTimeSynced() {
  struct tm timeinfo;
  g_timeSynced = getLocalTime(&timeinfo);
  return g_timeSynced;
}

uint32_t getUnixTimeUtc() {
  time_t now;
  time(&now);
  return static_cast<uint32_t>(now);
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