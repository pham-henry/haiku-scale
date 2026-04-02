#pragma once

// ===== HX711 Pins =====
constexpr int LOADCELL_DOUT_PIN = 16;
constexpr int LOADCELL_SCK_PIN  = 4;

// ===== Timing ===== // change to send data when there is a percentage weight change -> 5% 
constexpr unsigned long SAMPLE_INTERVAL_MS = 100;   // 10 Hz
constexpr unsigned long POST_INTERVAL_MS   = 1000;  // 1 Hz

// ===== Filtering =====
constexpr int   MOVING_AVG_WINDOW   = 10;
constexpr float CALIBRATION_FACTOR  = -471.497f;
constexpr float ZERO_CLAMP_THRESHOLD_G = 0.5f;

// ===== Time / NTP =====
constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr long GMT_OFFSET_SEC = 0;
constexpr int  DAYLIGHT_OFFSET_SEC = 0;