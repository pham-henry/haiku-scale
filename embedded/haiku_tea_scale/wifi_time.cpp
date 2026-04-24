/*
  wifi_time.cpp

  Manage Wi-Fi connectivity and UTC time synchronization.

  It keeps network recovery out of the sensing loop. Initial connection gets a
  short blocking attempt, while later reconnects are rate-limited so HX711
  sampling continues even when the cafe network or backend is unavailable.
*/

#include "wifi_time.h"

#include <WiFi.h>
#include "time.h"

static bool g_timeSynced = false;
static unsigned long lastReconnectAttemptMs = 0;
static unsigned long lastTimeRetryMs = 0;
static const unsigned long WIFI_RECONNECT_INTERVAL_MS = 15000;
static const unsigned long TIME_RETRY_INTERVAL_MS = 60000;

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

  unsigned long now = millis();
  if (now - lastReconnectAttemptMs < WIFI_RECONNECT_INTERVAL_MS) return;
  lastReconnectAttemptMs = now;

  Serial.println("WiFi disconnected. Starting reconnect attempt.");
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Reconnected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Reconnect started; continuing local sampling.");
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

void retryTimeSyncIfNeeded(const char* ntpServer, long gmtOffsetSec, int daylightOffsetSec) {
  if (g_timeSynced || WiFi.status() != WL_CONNECTED) return;

  unsigned long now = millis();
  if (now - lastTimeRetryMs < TIME_RETRY_INTERVAL_MS) return;
  lastTimeRetryMs = now;

  configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);
  struct tm timeinfo;
  g_timeSynced = getLocalTime(&timeinfo, 1000);
  Serial.println(g_timeSynced ? "Time sync recovered." : "Time sync retry failed.");
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
