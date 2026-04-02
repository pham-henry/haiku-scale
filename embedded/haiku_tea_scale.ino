#include <Arduino.h>

#include "config.h"
#include "secrets.h"
#include "wifi_time.h"
#include "backend_client.h"
#include "scale_sensor.h"

unsigned long lastSampleMs = 0;
unsigned long lastPostMs   = 0;

float currentWeight = 0.0f;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("Booting Haiku Tea Scale...");

  connectWiFi(WIFI_SSID, WIFI_PASSWORD);
  initTime(NTP_SERVER, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);
  initScale(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN, CALIBRATION_FACTOR);

  Serial.println("System ready.");
}

void loop() {
  unsigned long now = millis();

  maintainWiFi(WIFI_SSID, WIFI_PASSWORD);

  if (now - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = now;

    currentWeight = readFilteredWeight();

    Serial.print("Weight (g): ");
    Serial.println(currentWeight, 1);
  }

  if (now - lastPostMs >= POST_INTERVAL_MS) {
    lastPostMs = now;

    String timestamp = getIsoTimestampUtc();
    postMeasurement(SERVER_URL, currentWeight, timestamp);
  }
}