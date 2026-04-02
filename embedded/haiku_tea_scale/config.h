#pragma once

// ===== Device identity =====
constexpr const char* DEVICE_ID = "tea-stand-01";

// ===== HX711 pins =====
constexpr int LOADCELL_DOUT_PIN = 16;
constexpr int LOADCELL_SCK_PIN  = 4;

// ===== Calibration =====
// Replace with your final calibrated value for your real container setup.
constexpr float CALIBRATION_FACTOR = -35.8f;

// ===== Sampling / filtering =====
constexpr unsigned long SAMPLE_INTERVAL_MS = 100;   // 10 Hz
constexpr int MOVING_AVG_WINDOW = 10;

// ===== Reporting =====
constexpr unsigned long HEARTBEAT_INTERVAL_MS = 60000;   // 60 sec
constexpr float REPORT_DELTA_G = 50.0f;                  // send if weight changes by >= 50g

// ===== Stability / state thresholds =====
constexpr float ZERO_CLAMP_THRESHOLD_G = 5.0f;           // clamp tiny near-zero noise
constexpr float EMPTY_STAND_THRESHOLD_G = 30.0f;         // treat below this as empty stand
constexpr float CONTAINER_PRESENT_THRESHOLD_G = 100.0f;  // above this means container likely present
constexpr float STABLE_DELTA_G = 10.0f;                  // max change to still consider stable
constexpr unsigned long STABLE_TIME_MS = 1500;           // must remain stable this long

// ===== Time / NTP =====
constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr long GMT_OFFSET_SEC = 0;
constexpr int DAYLIGHT_OFFSET_SEC = 0;

// ===== Offline buffer =====
// 30 minutes at 1 sample/sec = 1800 entries
constexpr int BUFFER_SIZE = 1800;