# Haiku Tea Scale

Production-minded ESP32 + HX711 monitoring stand for Haiku Teahouse tea
containers. The stand measures supported-side grams from the slanted,
instrumented side of the platform; it does not measure total container weight
unless a later backend model transforms that signal.

## V2 Architecture

- `embedded/haiku_tea_scale/haiku_tea_scale.ino` coordinates sampling,
  state detection, reporting, buffering, Wi-Fi recovery, and debug logs.
- `scale_sensor.*` reads HX711 raw values and produces median-filtered,
  outlier-rejected, moving-average supported-side grams with variance health.
- `calibration_manager.*` persists calibration factor and zero offset in ESP32
  NVS, supports future two-point calibration, and prevents blind boot tare.
- `state_detector.*` maps filtered readings into physical cafe states such as
  empty stand, stable container, pouring, refill, temporary lift, and fault.
- `report_policy.*` decides when to send: boot, meaningful stable deltas,
  important state changes, critical events, and 5-minute heartbeats.
- `buffer_store.*` keeps at least 30 minutes of compact RAM outage events at
  1 Hz and flushes oldest-first after reconnection.
- `backend/tea-backend/server.js` stores ESP32 POSTs in SQLite and exposes
  health, latest, recent, per-device, and device-summary routes.

## Measurement Strategy

The firmware samples HX711 every 100 ms. Each raw sample is converted using the
persisted supported-side calibration, clamped near zero, screened for impossible
spikes, passed through a short median filter, and then smoothed with a 10-sample
moving average. Variance and bounded deltas drive stability, so bumps and
temporary lifts do not become false inventory changes.

The device never auto-tares on boot. A safe zero/baseline update is only allowed
when the stand is confidently empty, stable, and near zero for a dwell period.

## Reporting Strategy

The ESP32 does not stream every reading. It sends only when a stable weight
change exceeds the absolute gram threshold with hysteresis, an important state
changes, a critical event occurs, or a heartbeat interval elapses. During Wi-Fi
or backend outages it continues local sensing and stores compact events for
later flush.

## Tunable Per-Device Values

Edit `embedded/haiku_tea_scale/config.h` and `secrets.h` for deployment:

- `DEVICE_ID`, `FIRMWARE_VERSION`
- HX711 pins
- default calibration factor and persisted zero offset
- empty/container thresholds for supported-side grams
- stability variance and dwell thresholds
- report delta, hysteresis, rate limit, and heartbeat interval
- Wi-Fi credentials and backend URL

Backend routes:

- `GET /health`
- `POST /api/tea-weight`
- `GET /api/tea-weight?limit=50&device_id=tea-stand-01`
- `GET /api/tea-weight/latest?device_id=tea-stand-01`
- `GET /api/tea-weight/device/:deviceId`
- `GET /api/devices`
