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
syncfit::StatusLed g_status_led(pins::kStatusLed);
syncfit::WsClient g_ws_client;

syncfit::AppState g_state;
SemaphoreHandle_t g_state_mutex = nullptr;
SemaphoreHandle_t g_tx_mutex = nullptr;
char g_tx_buffer[6144];
bool g_tx_ready = false;
char g_device_id[32];
char g_session_id[17];
bool g_sensor_error = false;

void onWsMessage(const char* message) {
  Serial.printf("[WS] %s\n", message);
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
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
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
  syncfit::PeakDetector peak_detector;
  syncfit::RrMetrics rr_metrics;
  syncfit::RingBuffer<float, config::kPpgWindowSize> window;

  std::uint32_t last_peak_ms = 0;
  std::uint32_t last_temperature_ms = 0;
  std::uint32_t last_state_ms = 0;
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

    for (std::size_t i = 0; i < sample_count; ++i) {
      const float filtered = dc_blocker.process(ir_samples[i]);
      window.push(filtered);
      if (peak_detector.update(filtered, now)) {
        if (last_peak_ms != 0) rr_metrics.addRr(now - last_peak_ms);
        last_peak_ms = now;
      }
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
      g_state.heart_rate_bpm = rr_metrics.heartRateBpm();
      g_state.finger_present = finger_present;
      xSemaphoreGive(g_state_mutex);
    }
  }
}

void forceTask(void*) {
  std::uint32_t baseline_start_ms = millis();
  std::uint32_t last_button_ms = 0;
  float baseline_kg = 0.0f;
  bool baseline_ready = false;
  bool button_handled = false;
  bool last_button = false;

  TickType_t last_wake = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(config::kForcePeriodMs));
    const std::uint32_t now = millis();

    bool tare_requested = false;
    syncfit::provisioningConsumeTareRequest(tare_requested);
    float calibration_kg = 0.0f;
    bool calibration_requested = false;
    syncfit::provisioningConsumeCalibration(calibration_kg, calibration_requested);

#if SYNCFIT_MOCK_SENSORS
    if (tare_requested || calibration_requested) {
      Serial.println("[force] ignored in mock mode");
    }
#else
    if (tare_requested && g_hx711.tare(10)) {
      syncfit::provisioningSaveCalibration(g_hx711.offset(), g_hx711.scale());
    }
    if (calibration_requested && g_hx711.calibrate(calibration_kg, 10)) {
      syncfit::provisioningSaveCalibration(g_hx711.offset(), g_hx711.scale());
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
#endif

    if (force_ok) {
      if (!baseline_ready && now - baseline_start_ms < 5000) {
        if (force_kg > baseline_kg) baseline_kg = force_kg;
      } else if (!baseline_ready) {
        baseline_ready = true;
        Serial.printf("[force] baseline=%.2f kg\n", baseline_kg);
      }

      const float loss_pct =
          (baseline_ready && baseline_kg > 5.0f)
              ? clampf((baseline_kg - force_kg) / baseline_kg * 100.0f, 0.0f, 100.0f)
              : 0.0f;

      xSemaphoreTake(g_state_mutex, portMAX_DELAY);
      g_state.force_kg = force_kg;
      g_state.force_loss_pct = loss_pct;
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

void networkTask(void*) {
  std::uint32_t last_reconnect_ms = 0;
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

    g_ws_client.loop();
    g_status_led.setCloud(g_ws_client.connected());

    if (g_tx_ready) {
      xSemaphoreTake(g_tx_mutex, portMAX_DELAY);
      if (g_tx_ready && g_ws_client.connected() && g_ws_client.send(g_tx_buffer)) {
        g_tx_ready = false;
      }
      xSemaphoreGive(g_tx_mutex);
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
  xTaskCreatePinnedToCore(networkTask, "network", 4096, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(statusTask, "status", 2048, nullptr, 1, nullptr, 0);

  Serial.println("[boot] tasks started");
}

void loop() {
  syncfit::provisioningLoop();
  delay(20);
}
