// SyncFit Edge firmware entry point.
//
// Tasks:
//   sensorTask     100 Hz PPG + 1 Hz temperature
//   forceTask      10 SPS HX711 + tare/calibration button
//   telemetryTask  builds a contract-shaped frame every 2.56 s
//   networkTask    WebSocket loop, Wi-Fi reconnect and frame transmission
//   statusTask     status LED patterns
//
// Build with -DSYNCFIT_MOCK_SENSORS=1 to exercise the whole pipeline without
// connecting any sensor.

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <sys/time.h>
#include <esp_system.h>
#include <time.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "app_state.hpp"
#include "board_pins.hpp"
#include "config.hpp"
#include "drivers/hx711.hpp"
#include "drivers/max30102.hpp"
#include "drivers/mlx90614.hpp"
#include "filters.hpp"
#include "auto_tare.hpp"
#include "force_baseline.hpp"
#include "http_date.hpp"
#include "net/ws_client.hpp"
#include "provisioning.hpp"
#include "ring_buffer.hpp"
#include "status_led.hpp"
#include "telemetry_frame.hpp"

namespace {

using namespace syncfit;

syncfit::Max30102 g_max30102;
syncfit::Mlx90614 g_mlx90614;
syncfit::Hx711 g_hx711(pins::kHx711Dout, pins::kHx711Sck);
syncfit::MovingAverage<4> g_force_filter;
syncfit::StatusLed g_status_led(pins::kStatusLed);
syncfit::WsClient g_ws_client;

syncfit::AppState g_state;
// Set at boot: the force task zeroes a small drift as soon as the platform is
// at rest. Not on reconnects (the campus Wi-Fi drops often and a resting hand
// would become the new zero).
volatile bool g_autotare_request = true;
SemaphoreHandle_t g_state_mutex = nullptr;
SemaphoreHandle_t g_tx_mutex = nullptr;
char g_tx_buffer[6144];
bool g_tx_ready = false;
char g_device_id[32];
char g_session_id[17];
bool g_sensor_error = false;

void onWsMessage(const char* message) {
  Serial.printf("[WS] %s\n", message);
  if (strstr(message, "\"action\":\"tare\"") != nullptr ||
      strstr(message, "\"command\":\"tare\"") != nullptr) {
    provisioningRequestTare();
    Serial.println("[WS] tare queued");
  }
}

void buildIds() {
  WiFi.mode(WIFI_STA);
  std::uint8_t mac[6] = {};
  WiFi.macAddress(mac);
  std::snprintf(g_device_id, sizeof(g_device_id), "syncfit-esp32-%02X%02X%02X", mac[3],
                mac[4], mac[5]);
  std::snprintf(g_session_id, sizeof(g_session_id), "%08lX",
                static_cast<unsigned long>(esp_random()));
}

void connectWifi() {
  const auto& cfg = syncfit::deviceConfig();
  if (cfg.wifi_ssid.isEmpty()) {
    Serial.println("[wifi] no SSID configured; use SET WIFI <ssid>,<password>");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.begin(cfg.wifi_ssid.c_str(), cfg.wifi_password.c_str());

  Serial.printf("[wifi] connecting to %s", cfg.wifi_ssid.c_str());
  for (int i = 0; i < 40 && WiFi.status() != WL_CONNECTED; ++i) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[wifi] connected: %s\n", WiFi.localIP().toString().c_str());
    configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
  } else {
    Serial.println("[wifi] not connected; telemetry will wait");
  }
}

#if SYNCFIT_MOCK_SENSORS
float mockPpg(std::uint32_t now_ms) {
  const float seconds = static_cast<float>(now_ms) / 1000.0f;
  const float beat = std::sin(2.0f * kPi * 1.2f * seconds);
  const float noise = 120.0f * std::sin(2.0f * kPi * 7.0f * seconds);
  return 50000.0f + 4500.0f * beat + noise;
}

float mockTemperature(std::uint32_t now_ms) {
  const float seconds = static_cast<float>(now_ms) / 1000.0f;
  return 36.8f + 0.15f * std::sin(2.0f * kPi * 0.01f * seconds);
}

float mockForce(std::uint32_t now_ms) {
  const float seconds = static_cast<float>(now_ms) / 1000.0f;
  return 12.0f + 6.0f * std::sin(2.0f * kPi * 0.05f * seconds);
}
#endif

void sensorTask(void*) {
  syncfit::DcBlocker dc_blocker;
  // The first blocker removes the large optical DC offset. The second one
  // removes the ~0.4 Hz wander caused by finger pressure and breathing, which
  // otherwise dominates the envelope and makes the peak detector fire on it.
  syncfit::DcBlocker wander_blocker(0.95f);
  // A short moving average removes the high-frequency noise that made the
  // peak detector fire on every wiggle. At 100 Hz, 8 samples is 80 ms: it
  // smooths above ~6 Hz without attenuating the 1–2 Hz pulse.
  syncfit::MovingAverage<8> ppg_smoother;
  syncfit::PeakDetector peak_detector;
  syncfit::MedianFilter<51> hr_filter;  // ~5 s at the 100 ms state cadence: one bad window cannot move it
  syncfit::SampleRateMeter rate_meter;  // real MAX30102 rate (nominal 100 sps)
  syncfit::RrMetrics rr_metrics;
  syncfit::RingBuffer<float, config::kPpgWindowSize> window;

  std::uint32_t last_peak_ms = 0;
  std::uint32_t last_temperature_ms = 0;
  std::uint32_t last_state_ms = 0;
  std::uint32_t last_ppg_log_ms = 0;
  std::uint32_t last_finger_ms = 0;
  float temperature_baseline = NAN;

  TickType_t last_wake = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
    const std::uint32_t now = millis();

    float ir_samples[8] = {};
    float red_samples[8] = {};
    std::size_t sample_count = 0;
    bool finger_present = false;

#if SYNCFIT_MOCK_SENSORS
    ir_samples[0] = mockPpg(now);
    red_samples[0] = ir_samples[0] * 0.8f;
    sample_count = 1;
    finger_present = true;
#else
    if (g_max30102.readFifo(ir_samples, red_samples, 8, sample_count)) {
      for (std::size_t i = 0; i < sample_count; ++i) {
        if (ir_samples[i] > 5000.0f) finger_present = true;
      }
    }
#endif

    if (rate_meter.add(sample_count, now)) {
      static std::uint32_t last_rate_log_ms = 0;
      if (now - last_rate_log_ms >= 10000) {
        last_rate_log_ms = now;
        Serial.printf("[ppg] rate=%.1f Hz\n", rate_meter.rate());
      }
    }

    if (finger_present) last_finger_ms = now;
    // Keep the last valid reading for a moment: FIFO reads can occasionally
    // return no samples even while the finger is still on the sensor.
    const bool finger_active =
        last_finger_ms != 0 && (now - last_finger_ms) < 300u;
    if (!finger_active) {
      peak_detector.reset();
      rr_metrics.reset();
      last_peak_ms = 0;
    }

    for (std::size_t i = 0; i < sample_count; ++i) {
      const float filtered = dc_blocker.process(ir_samples[i]);
      const float pulsatile = wander_blocker.process(filtered);
      window.push(pulsatile);
      if (!finger_active) continue;
      const float smoothed = ppg_smoother.push(pulsatile);
      // A FIFO read can return several samples at once; give each one its
      // real 10 ms timestamp so RR intervals are not quantised to the task tick.
      const std::uint32_t sample_ms =
          now - static_cast<std::uint32_t>((sample_count - 1 - i) * (1000.0f / rate_meter.rate()));
      if (peak_detector.update(smoothed, sample_ms)) {
        if (last_peak_ms != 0) rr_metrics.addRr(sample_ms - last_peak_ms);
        last_peak_ms = sample_ms;
      }
    }

    bool dump_ppg = false;
    provisioningConsumePpgDump(dump_ppg);
    if (dump_ppg) {
      Serial.printf("ppg_window count=%u finger=%d\n",
                    static_cast<unsigned>(window.size()), finger_active ? 1 : 0);
      for (std::size_t i = 0; i < window.size(); ++i) {
        Serial.printf("%.1f\n", window.at(i));
      }
      Serial.println("ppg_window_end");
    }

    if (now - last_temperature_ms >= config::kTemperaturePeriodMs) {
      last_temperature_ms = now;
      float temperature_c = 0.0f;
      bool temperature_ok = false;

#if SYNCFIT_MOCK_SENSORS
      temperature_c = mockTemperature(now);
      temperature_ok = true;
#else
      temperature_ok = g_mlx90614.readObjectC(temperature_c);
#endif

      if (temperature_ok) {
        if (std::isnan(temperature_baseline)) temperature_baseline = temperature_c;
        xSemaphoreTake(g_state_mutex, portMAX_DELAY);
        g_state.object_temperature_c = temperature_c;
        g_state.delta_temperature_c =
            clampf(temperature_c - temperature_baseline, -2.0f, 2.0f);
        g_state.temperature_valid = true;
        xSemaphoreGive(g_state_mutex);
      }
    }

    if (now - last_state_ms >= 100) {
      last_state_ms = now;
      xSemaphoreTake(g_state_mutex, portMAX_DELAY);
      g_state.ppg_count = window.size();
      for (std::size_t i = 0; i < window.size(); ++i) {
        g_state.ppg[i] = window.at(i);
      }
      g_state.rmssd_ms = rr_metrics.rmssdMs();
      // Prefer the autocorrelation estimate: it is not fooled by the dicrotic
      // notch the way the peak detector is. A 2 s median rejects the occasional
      // octave error (40/197 bpm).
      if (!finger_active) {
        hr_filter.reset();
      } else {
        const float autocorr_hr =
            estimateHeartRateBpm(g_state.ppg, g_state.ppg_count, rate_meter.rate());
        if (autocorr_hr > 0.0f) hr_filter.push(autocorr_hr);
      }
      const float filtered_hr = hr_filter.value();
      // No fallback to the peak detector: it counts the dicrotic notch as a
      // second beat (double pulse). Until a real estimate exists show "—".
      g_state.heart_rate_bpm = filtered_hr > 0.0f ? filtered_hr : 0.0f;
      g_state.finger_present = finger_active;
      xSemaphoreGive(g_state_mutex);
    }

    if (now - last_ppg_log_ms >= 1000) {
      last_ppg_log_ms = now;
      xSemaphoreTake(g_state_mutex, portMAX_DELAY);
      const bool present = g_state.finger_present;
      const float hr = g_state.heart_rate_bpm;
      const float temp = g_state.object_temperature_c;
      const float delta = g_state.delta_temperature_c;
      const bool temp_valid = g_state.temperature_valid;
      const std::size_t count = g_state.ppg_count;
      xSemaphoreGive(g_state_mutex);
      Serial.printf("[ppg] finger=%d hr=%.0f samples=%u temp=%s%.2f dt=%.2f\n",
                    present ? 1 : 0, hr, static_cast<unsigned>(count),
                    temp_valid ? "" : "?", temp, delta);
    }
  }
}

void forceTask(void*) {
  std::uint32_t last_button_ms = 0;
  syncfit::ForceBaseline force_baseline;
  syncfit::AutoTareGate auto_tare;  // 3 s still, < 6 kg, gives up after 60 s
  bool button_handled = false;
  bool last_button = false;

  TickType_t last_wake = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(config::kForcePeriodMs));
    const std::uint32_t now = millis();

    bool tare_requested = false;
    syncfit::provisioningConsumeTareRequest(tare_requested);
    if (g_autotare_request) {
      g_autotare_request = false;
      auto_tare.arm(now);
      Serial.println("[force] auto-tare armed: keep the platform still");
    }
    float calibration_kg = 0.0f;
    bool calibration_requested = false;
    syncfit::provisioningConsumeCalibration(calibration_kg, calibration_requested);
    bool start_requested = false;
    syncfit::provisioningConsumeForceStart(start_requested);
    if (start_requested) {
      force_baseline.reset();
      Serial.println("[force] set reset; push >=5 kg");
    }

#if SYNCFIT_MOCK_SENSORS
    if (tare_requested || calibration_requested) {
      Serial.println("[force] ignored in mock mode");
    }
#else
    if (tare_requested) {
      if (g_hx711.tare(10)) {
        g_force_filter.reset();
        syncfit::provisioningSaveCalibration(g_hx711.offset(), g_hx711.scale());
        Serial.printf("[force] tare ok offset=%ld\n",
                      static_cast<long>(g_hx711.offset()));
      } else {
        Serial.println("[force] tare failed: HX711 not responding");
      }
    }
    if (calibration_requested) {
      if (g_hx711.calibrate(calibration_kg, 10)) {
        syncfit::provisioningSaveCalibration(g_hx711.offset(), g_hx711.scale());
        Serial.printf("[force] cal ok scale=%.1f counts/kg\n", g_hx711.scale());
      } else {
        Serial.println("[force] cal failed: no change or HX711 not responding");
      }
    }
#endif

    const bool button = digitalRead(pins::kTareButton) == LOW;
    if (button && !last_button) last_button_ms = now;
    if (button && !button_handled && now - last_button_ms > 50) {
#if SYNCFIT_MOCK_SENSORS
      Serial.println("[force] tare button pressed (mock)");
#else
      if (g_hx711.tare(10)) {
        syncfit::provisioningSaveCalibration(g_hx711.offset(), g_hx711.scale());
      }
#endif
      button_handled = true;
    }
    if (!button) button_handled = false;
    last_button = button;

    float force_kg = 0.0f;
    bool force_ok = false;

#if SYNCFIT_MOCK_SENSORS
    force_kg = mockForce(now);
    force_ok = true;
#else
    force_ok = g_hx711.readKg(force_kg);
    if (force_ok) {
      force_kg = g_force_filter.push(force_kg);
    } else if (g_hx711.scale() == 0.0f) {
      static std::uint32_t last_raw_ms = 0;
      if (now - last_raw_ms >= 1000) {
        last_raw_ms = now;
        Serial.printf("[force] raw=%ld ready=%d (sin calibrar)\n",
                      static_cast<long>(g_hx711.lastRaw()),
                      g_hx711.isReady() ? 1 : 0);
      }
    }
#endif

#if !SYNCFIT_MOCK_SENSORS
    // Only a small drift is zeroed: a hand resting on the platform is a load.
    if (force_ok && std::fabs(force_kg) < 1.5f && auto_tare.push(force_kg, now)) {
      const float drift = force_kg;
      if (g_hx711.tare(10)) {
        g_force_filter.reset();
        force_baseline.reset();
        syncfit::provisioningSaveCalibration(g_hx711.offset(), g_hx711.scale());
        Serial.printf("[force] auto-tare ok (drift %.2f kg) offset=%ld\n", drift,
                      static_cast<long>(g_hx711.offset()));
        force_kg = 0.0f;
      }
    }
#endif

    if (force_ok) {
      force_baseline.update(force_kg, now);
      static std::uint32_t last_force_log_ms = 0;
      if (now - last_force_log_ms >= 1000) {
        last_force_log_ms = now;
        Serial.printf("[force] kg=%.2f loss=%.1f%% baseline=%.2f\n",
                      force_kg, force_baseline.lossPct(),
                      force_baseline.baselineKg());
      }

      xSemaphoreTake(g_state_mutex, portMAX_DELAY);
      g_state.force_kg = force_kg;
      g_state.force_loss_pct = force_baseline.lossPct();
      g_state.force_valid = true;
      xSemaphoreGive(g_state_mutex);
    }
  }
}

void telemetryTask(void*) {
  TickType_t last_wake = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(config::kTelemetryPeriodMs));

    time_t now = time(nullptr);
#if SYNCFIT_MOCK_SENSORS
    // Mock mode can run on the bench without Wi-Fi/NTP.
    if (now < 1700000000) now = 1760000000 + static_cast<time_t>(millis() / 1000);
#else
    if (now < 1700000000) {
      Serial.println("[telemetry] waiting for NTP time");
      continue;
    }
#endif

    struct tm utc = {};
    gmtime_r(&now, &utc);
    char timestamp[25] = {};
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &utc);

    syncfit::AppState snapshot;
    xSemaphoreTake(g_state_mutex, portMAX_DELAY);
    snapshot = g_state;
    xSemaphoreGive(g_state_mutex);

    if (snapshot.ppg_count == 0) continue;

    const auto& cfg = syncfit::deviceConfig();
    syncfit::TelemetryData frame;
    frame.device_id = g_device_id;
    frame.session_id = g_session_id;
    frame.timestamp = timestamp;
    frame.modality = cfg.modality.c_str();
    frame.day_or_week = cfg.day_or_week;
    frame.delta_temperature_c = snapshot.delta_temperature_c;
    frame.rmssd_hrv_ms = snapshot.rmssd_ms;
    frame.isometric_force_loss_pct = snapshot.force_loss_pct;
    frame.heart_rate_bpm = snapshot.heart_rate_bpm;
    frame.force_kg = snapshot.force_kg;
    frame.object_temperature_c = snapshot.object_temperature_c;
    frame.finger_present = snapshot.finger_present;
    frame.ppg_samples = snapshot.ppg;
    frame.ppg_count = snapshot.ppg_count;

    xSemaphoreTake(g_tx_mutex, portMAX_DELAY);
    const bool built = syncfit::buildTelemetryJson(frame, g_tx_buffer, sizeof(g_tx_buffer));
    if (built) {
      g_tx_ready = true;
#if SYNCFIT_MOCK_SENSORS
      Serial.printf("[telemetry] %s\n", g_tx_buffer);
#endif
    } else {
      Serial.println("[telemetry] frame buffer too small");
    }
    xSemaphoreGive(g_tx_mutex);
  }
}

// Host of the telemetry API, reused as an HTTPS clock when NTP is blocked.
String g_clock_host;

bool clockIsSet() { return time(nullptr) >= 1700000000; }

// Campus networks often block NTP (UDP 123). Any HTTPS answer from our API
// carries a `Date` header: use it to set the clock (second precision is
// plenty for frame timestamps). Blocking, so only called while the clock is unset.
bool setClockFromHttps() {
  if (g_clock_host.isEmpty()) return false;
  WiFiClientSecure client;
  client.setInsecure();  // only the Date header is read; no data is trusted
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(client, String("https://") + g_clock_host + "/api/v1/health")) return false;
  const char* keys[] = {"Date"};
  http.collectHeaders(keys, 1);
  const int code = http.GET();
  std::int64_t epoch = 0;
  const bool ok = code > 0 && syncfit::parseHttpDate(http.header("Date").c_str(), epoch);
  http.end();
  if (!ok) {
    Serial.printf("[clock] https fallback failed (code %d)\n", code);
    return false;
  }
  timeval tv = {static_cast<time_t>(epoch), 0};
  settimeofday(&tv, nullptr);
  Serial.printf("[clock] set from HTTPS Date: %lld\n", static_cast<long long>(epoch));
  return true;
}

void networkTask(void*) {
  std::uint32_t last_reconnect_ms = 0;
  std::uint32_t last_force_ms = 0;
  bool ntp_started = false;
  std::uint32_t wifi_up_ms = 0;
  std::uint32_t last_clock_try_ms = 0;
  char force_buffer[96];
  syncfit::ForceSendGate force_gate;
  TickType_t last_wake = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
    const std::uint32_t now = millis();
    const bool wifi_connected = WiFi.status() == WL_CONNECTED;
    g_status_led.setWifi(wifi_connected);

    if (!wifi_connected && now - last_reconnect_ms > 5000) {
      last_reconnect_ms = now;
      WiFi.reconnect();
    }

    // Clock: start NTP whenever Wi-Fi comes up (not only during setup), and
    // fall back to the API's HTTPS Date header if NTP stays silent.
    if (wifi_connected) {
      if (!ntp_started) {
        configTime(0, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
        ntp_started = true;
        wifi_up_ms = now;
      }
      if (!clockIsSet() && now - wifi_up_ms > 10000 && now - last_clock_try_ms > 15000) {
        last_clock_try_ms = now;
        setClockFromHttps();
      }
    } else {
      ntp_started = false;
    }

    g_ws_client.loop();

    g_status_led.setCloud(g_ws_client.connected());

    bool sent_frame = false;
    if (g_tx_ready) {
      xSemaphoreTake(g_tx_mutex, portMAX_DELAY);
      if (g_tx_ready && g_ws_client.connected() && g_ws_client.send(g_tx_buffer)) {
        g_tx_ready = false;
        sent_frame = true;
      }
      xSemaphoreGive(g_tx_mutex);
    }

    // Fast force channel: up to every 100 ms while the force changes, 1 Hz
    // keep-alive when idle (the full frame takes 2.56 s).
    // Never in the same pass as the 2.56 s frame: back-to-back TLS writes
    // starve loop() and the pong is missed.
    if (!sent_frame && g_ws_client.connected() && now - last_force_ms >= config::kForceSendIntervalMs) {
      last_force_ms = now;
      float force_kg = 0.0f;
      float loss_pct = 0.0f;
      bool valid = false;
      xSemaphoreTake(g_state_mutex, portMAX_DELAY);
      valid = g_state.force_valid;
      force_kg = g_state.force_kg;
      loss_pct = g_state.force_loss_pct;
      xSemaphoreGive(g_state_mutex);
      if (valid && force_gate.shouldSend(force_kg, loss_pct, now, 200) &&
          syncfit::buildForceJson(force_kg, loss_pct, force_buffer, sizeof(force_buffer))) {
        g_ws_client.send(force_buffer);
      }
    }
  }
}

void statusTask(void*) {
  for (;;) {
    g_status_led.update(millis());
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  syncfit::provisioningBegin();
  buildIds();
  Serial.printf("\nSyncFit Edge firmware %s | device=%s session=%s\n",
                config::kFirmwareVersion, g_device_id, g_session_id);

  Wire.begin(pins::kI2cSda, pins::kI2cScl);
  Wire.setClock(pins::kI2cClockHz);

  bool max_ok = true;
  bool mlx_ok = true;
#if !SYNCFIT_MOCK_SENSORS
  max_ok = g_max30102.begin(Wire);
  mlx_ok = g_mlx90614.begin(Wire);
  Serial.printf("[i2c] MAX30102=%s MLX90614=%s\n", max_ok ? "ok" : "missing",
                mlx_ok ? "ok" : "missing");
#endif

  g_hx711.begin();
  // The tare button is optional. Pull-up keeps GPIO4 high when it is absent;
  // use the serial command TARE instead.
  pinMode(pins::kTareButton, INPUT_PULLUP);
  const auto& cfg = syncfit::deviceConfig();
  g_hx711.setOffset(cfg.hx_offset);
  g_hx711.setScale(cfg.hx_scale);
  g_status_led.begin();
  g_sensor_error = !(max_ok && mlx_ok);
  g_status_led.setError(g_sensor_error);

  connectWifi();
  g_clock_host = cfg.ws_host;
  if (!cfg.ws_host.isEmpty()) {
    g_ws_client.begin(cfg.ws_host.c_str(), cfg.ws_port, cfg.ws_path.c_str(),
                      cfg.ws_secure);
    g_ws_client.setMessageHandler(onWsMessage);
  }

  g_state_mutex = xSemaphoreCreateMutex();
  g_tx_mutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(sensorTask, "sensor", 4096, nullptr, 5, nullptr, 1);
  xTaskCreatePinnedToCore(forceTask, "force", 4096, nullptr, 3, nullptr, 0);
  xTaskCreatePinnedToCore(telemetryTask, "telemetry", 8192, nullptr, 2, nullptr, 0);
  // 8 KB: the WebSocket handshake plus a ~6 KB telemetry frame overflowed 4 KB.
  xTaskCreatePinnedToCore(networkTask, "network", 8192, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(statusTask, "status", 2048, nullptr, 1, nullptr, 0);

  Serial.println("[boot] tasks started");
}

void loop() {
  syncfit::provisioningLoop();
  delay(20);
}
