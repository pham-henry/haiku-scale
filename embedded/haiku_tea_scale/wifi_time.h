#pragma once

#include <Arduino.h>

void connectWiFi(const char* ssid, const char* password);
void maintainWiFi(const char* ssid, const char* password);

void initTime(const char* ntpServer, long gmtOffsetSec, int daylightOffsetSec);
void retryTimeSyncIfNeeded(const char* ntpServer, long gmtOffsetSec, int daylightOffsetSec);
bool isTimeSynced();
uint32_t getUnixTimeUtc();
String getIsoTimestampUtc();
