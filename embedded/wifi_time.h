#pragma once

#include <Arduino.h>

void connectWiFi(const char* ssid, const char* password);
void maintainWiFi(const char* ssid, const char* password);

void initTime(const char* ntpServer, long gmtOffsetSec, int daylightOffsetSec);
String getIsoTimestampUtc();