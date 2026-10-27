// Serial provisioning and NVS persistence.
//
// Commands:
//   HELP
//   STATUS
//   SET MODALITY MENSTRUAL_CYCLE|GESTATIONAL
//   SET WEEK 1..42
//   SET WIFI <ssid>,<password>
//   SET WS ws://host:port/path  (or wss://...)
//   TARE
//   CAL <known_kg>
//   REBOOT

#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include <cstdint>

namespace syncfit {

struct DeviceConfig {
  String wifi_ssid;
  String wifi_password;
  String ws_host;
  std::uint16_t ws_port = 8000;
  String ws_path = "/api/v1/ws/telemetry";
  bool ws_secure = false;
  String modality = "MENSTRUAL_CYCLE";
  std::uint8_t day_or_week = 14;
  float hx_scale = 0.0f;  // 0 = not calibrated; force readings are ignored
  std::int32_t hx_offset = 0;
};

void provisioningBegin();
void provisioningLoop();
const DeviceConfig& deviceConfig();

void provisioningSaveCalibration(std::int32_t offset, float scale);
void provisioningConsumeTareRequest(bool& requested);
void provisioningConsumeCalibration(float& known_kg, bool& requested);

}  // namespace syncfit
