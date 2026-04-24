#pragma once

// ===== Device identity =====
constexpr const char* DEVICE_ID = "tea-stand-01";
constexpr const char* FIRMWARE_VERSION = "2.0.0";

// ===== HX711 pins =====
constexpr int LOADCELL_DOUT_PIN = 16;
constexpr int LOADCELL_SCK_PIN  = 4;

// ===== Calibration =====
// This is supported-side grams, not total container grams.
constexpr float DEFAULT_CALIBRATION_FACTOR = -35.8f;
constexpr long DEFAULT_ZERO_OFFSET_RAW = 0;
constexpr const char* CALIBRATION_NAMESPACE = "tea-scale";

// ===== Sampling / filtering =====
constexpr unsigned long SAMPLE_INTERVAL_MS = 100;   // 10 Hz
constexpr int MOVING_AVG_WINDOW = 10;
constexpr int MEDIAN_WINDOW = 5;
constexpr float OUTLIER_REJECT_DELTA_G = 180.0f;
constexpr float IMPOSSIBLE_SPIKE_DELTA_G = 500.0f;
constexpr float SIGNAL_FAULT_VARIANCE_G2 = 25000.0f;

// ===== Reporting =====
constexpr unsigned long HEARTBEAT_INTERVAL_MS = 5UL * 60UL * 1000UL;
constexpr unsigned long MIN_REPORT_INTERVAL_MS = 10UL * 1000UL;
constexpr unsigned long STABLE_REPORT_DWELL_MS = 2500;
constexpr float REPORT_DELTA_G = 45.0f;
constexpr float REPORT_HYSTERESIS_G = 10.0f;
constexpr float MAJOR_REFILL_DELTA_G = 250.0f;

// ===== Stability / state thresholds =====
constexpr float ZERO_CLAMP_THRESHOLD_G = 5.0f;           // clamp tiny near-zero noise
constexpr float EMPTY_STAND_THRESHOLD_G = 30.0f;         // treat below this as empty stand
constexpr float CONTAINER_PRESENT_THRESHOLD_G = 100.0f;  // above this means container likely present
constexpr float STABLE_DELTA_G = 10.0f;                  // max change to still consider stable
constexpr float STABLE_VARIANCE_G2 = 64.0f;
constexpr unsigned long STABLE_TIME_MS = 1500;           // must remain stable this long
constexpr unsigned long TEMPORARY_LIFT_WINDOW_MS = 8000;

// ===== Drift / tare safety =====
constexpr unsigned long EMPTY_ZERO_DWELL_MS = 15000;
constexpr float SAFE_TARE_MAX_WEIGHT_G = 20.0f;
constexpr float DRIFT_CORRECTION_MAX_STEP_G = 1.0f;

// ===== Time / NTP =====
constexpr const char* NTP_SERVER = "pool.ntp.org";
constexpr long GMT_OFFSET_SEC = 0;
constexpr int DAYLIGHT_OFFSET_SEC = 0;

// ===== Offline buffer =====
// 30 minutes at 1 sample/sec = 1800 entries
constexpr int BUFFER_SIZE = 1800;
constexpr unsigned long OFFLINE_BUFFER_INTERVAL_MS = 1000;
constexpr int FLUSH_BATCH_LIMIT = 25;
