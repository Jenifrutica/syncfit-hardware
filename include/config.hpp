// Build-time defaults. Runtime values (Wi-Fi, WebSocket, modality, calibration)
// live in NVS and can be changed over the serial console.

#pragma once

#include <cstddef>
#include <cstdint>

// Optional local overrides; never commit real credentials.
#if __has_include("secrets.hpp")
#include "secrets.hpp"
#endif

#ifndef SYNCFIT_FIRMWARE_VERSION
#define SYNCFIT_FIRMWARE_VERSION "0.1.0"
#endif

#ifndef SYNCFIT_WIFI_SSID
#define SYNCFIT_WIFI_SSID ""
#endif

#ifndef SYNCFIT_WIFI_PASSWORD
#define SYNCFIT_WIFI_PASSWORD ""
#endif

#ifndef SYNCFIT_WS_HOST
#define SYNCFIT_WS_HOST "192.168.1.100"
#endif

#ifndef SYNCFIT_WS_PORT
#define SYNCFIT_WS_PORT 8000
#endif

#ifndef SYNCFIT_WS_PATH
#define SYNCFIT_WS_PATH "/api/v1/ws/telemetry"
#endif

#ifndef SYNCFIT_WS_SECURE
#define SYNCFIT_WS_SECURE 0
#endif

#ifndef SYNCFIT_PPG_SAMPLE_RATE_HZ
#define SYNCFIT_PPG_SAMPLE_RATE_HZ 100
#endif

#ifndef SYNCFIT_PPG_WINDOW_SIZE
#define SYNCFIT_PPG_WINDOW_SIZE 256
#endif

#ifndef SYNCFIT_TELEMETRY_PERIOD_MS
#define SYNCFIT_TELEMETRY_PERIOD_MS 2560  // 256 samples at 100 Hz
#endif

#ifndef SYNCFIT_FORCE_PERIOD_MS
#define SYNCFIT_FORCE_PERIOD_MS 100  // HX711 at 10 SPS
#endif

#ifndef SYNCFIT_TEMPERATURE_PERIOD_MS
#define SYNCFIT_TEMPERATURE_PERIOD_MS 1000
#endif

#ifndef SYNCFIT_MOCK_SENSORS
#define SYNCFIT_MOCK_SENSORS 0
#endif

#ifndef SYNCFIT_FORMAT_B
#define SYNCFIT_FORMAT_B 0
#endif

namespace syncfit::config {

constexpr char kFirmwareVersion[] = SYNCFIT_FIRMWARE_VERSION;
constexpr char kDefaultWifiSsid[] = SYNCFIT_WIFI_SSID;
constexpr char kDefaultWifiPassword[] = SYNCFIT_WIFI_PASSWORD;
constexpr char kDefaultWsHost[] = SYNCFIT_WS_HOST;
constexpr std::uint16_t kDefaultWsPort = SYNCFIT_WS_PORT;
constexpr char kDefaultWsPath[] = SYNCFIT_WS_PATH;
constexpr bool kDefaultWsSecure = SYNCFIT_WS_SECURE != 0;

constexpr std::uint32_t kPpgSampleRateHz = SYNCFIT_PPG_SAMPLE_RATE_HZ;
constexpr std::size_t kPpgWindowSize = SYNCFIT_PPG_WINDOW_SIZE;
constexpr std::uint32_t kTelemetryPeriodMs = SYNCFIT_TELEMETRY_PERIOD_MS;
constexpr std::uint32_t kForcePeriodMs = SYNCFIT_FORCE_PERIOD_MS;
constexpr std::uint32_t kTemperaturePeriodMs = SYNCFIT_TEMPERATURE_PERIOD_MS;

constexpr bool kMockSensors = SYNCFIT_MOCK_SENSORS != 0;
constexpr bool kFormatB = SYNCFIT_FORMAT_B != 0;

constexpr char kDefaultModality[] = "MENSTRUAL_CYCLE";
constexpr std::uint8_t kDefaultDayOrWeek = 14;

}  // namespace syncfit::config
