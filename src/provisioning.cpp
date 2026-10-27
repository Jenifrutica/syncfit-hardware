#include "provisioning.hpp"

#include <cstdlib>

#include "config.hpp"

namespace syncfit {
namespace {
Preferences g_prefs;
DeviceConfig g_config;
String g_line;
bool g_tare_requested = false;
bool g_cal_requested = false;
float g_cal_kg = 0.0f;

void printHelp() {
  Serial.println("Commands:");
  Serial.println("  HELP");
  Serial.println("  STATUS");
  Serial.println("  SET MODALITY MENSTRUAL_CYCLE|GESTATIONAL");
  Serial.println("  SET WEEK 1..42");
  Serial.println("  SET WIFI <ssid>,<password>");
  Serial.println("  SET WS ws://host:port/path");
  Serial.println("  TARE");
  Serial.println("  CAL <known_kg>");
  Serial.println("  REBOOT");
}

void printStatus() {
  Serial.printf("firmware=%s device=%s\n", config::kFirmwareVersion, "syncfit-esp32");
  Serial.printf("wifi_ssid=%s\n", g_config.wifi_ssid.c_str());
  Serial.printf("ws=%s://%s:%u%s\n", g_config.ws_secure ? "wss" : "ws",
                g_config.ws_host.c_str(), g_config.ws_port, g_config.ws_path.c_str());
  Serial.printf("modality=%s day_or_week=%u\n", g_config.modality.c_str(),
                g_config.day_or_week);
  Serial.printf("hx_offset=%ld hx_scale=%.4f\n", static_cast<long>(g_config.hx_offset),
                g_config.hx_scale);
}

bool parseWsUrl(const String& value, String& host, std::uint16_t& port, String& path,
                bool& secure) {
  String rest = value;
  rest.trim();
  secure = rest.startsWith("wss://");
  if (secure) {
    rest.remove(0, 6);
  } else if (rest.startsWith("ws://")) {
    rest.remove(0, 5);
  } else {
    return false;
  }

  const int slash = rest.indexOf('/');
  const String host_port = slash >= 0 ? rest.substring(0, slash) : rest;
  path = slash >= 0 ? rest.substring(slash) : "/api/v1/ws/telemetry";
  if (host_port.isEmpty()) return false;

  const int colon = host_port.lastIndexOf(':');
  if (colon > 0) {
    host = host_port.substring(0, colon);
    port = static_cast<std::uint16_t>(host_port.substring(colon + 1).toInt());
  } else {
    host = host_port;
    port = secure ? 443 : 80;
  }
  return !host.isEmpty() && port > 0;
}

void handleLine(String line) {
  line.trim();
  if (line.isEmpty()) return;

  if (line == "HELP") {
    printHelp();
    return;
  }
  if (line == "STATUS") {
    printStatus();
    return;
  }
  if (line == "TARE") {
    g_tare_requested = true;
    Serial.println("tare queued");
    return;
  }
  if (line == "REBOOT") {
    Serial.println("rebooting");
    delay(100);
    ESP.restart();
    return;
  }
  if (line.startsWith("CAL ")) {
    const float kg = line.substring(4).toFloat();
    if (kg > 0.0f) {
      g_cal_kg = kg;
      g_cal_requested = true;
      Serial.printf("calibration queued: %.2f kg\n", kg);
    } else {
      Serial.println("CAL requires a positive weight");
    }
    return;
  }
  if (line.startsWith("SET MODALITY ")) {
    String value = line.substring(13);
    value.trim();
    if (value == "MENSTRUAL_CYCLE" || value == "GESTATIONAL") {
      g_config.modality = value;
      g_prefs.putString("modality", value);
      Serial.printf("modality=%s\n", value.c_str());
    } else {
      Serial.println("modality must be MENSTRUAL_CYCLE or GESTATIONAL");
    }
    return;
  }
  if (line.startsWith("SET WEEK ")) {
    const int week = line.substring(9).toInt();
    if (week >= 1 && week <= 42) {
      g_config.day_or_week = static_cast<std::uint8_t>(week);
      g_prefs.putUChar("day", g_config.day_or_week);
      Serial.printf("day_or_week=%d\n", week);
    } else {
      Serial.println("week must be 1..42");
    }
    return;
  }
  if (line.startsWith("SET WIFI ")) {
    const String rest = line.substring(9);
    const int comma = rest.indexOf(',');
    if (comma > 0) {
      g_config.wifi_ssid = rest.substring(0, comma);
      g_config.wifi_password = rest.substring(comma + 1);
      g_prefs.putString("wifi_ssid", g_config.wifi_ssid);
      g_prefs.putString("wifi_pass", g_config.wifi_password);
      Serial.printf("wifi_ssid=%s (reboot to reconnect)\n", g_config.wifi_ssid.c_str());
    } else {
      Serial.println("use SET WIFI <ssid>,<password>");
    }
    return;
  }
  if (line.startsWith("SET WS ")) {
    String host;
    std::uint16_t port = 0;
    String path;
    bool secure = false;
    if (parseWsUrl(line.substring(7), host, port, path, secure)) {
      g_config.ws_host = host;
      g_config.ws_port = port;
      g_config.ws_path = path;
      g_config.ws_secure = secure;
      g_prefs.putString("ws_host", host);
      g_prefs.putUShort("ws_port", port);
      g_prefs.putString("ws_path", path);
      g_prefs.putBool("ws_secure", secure);
      Serial.printf("ws=%s://%s:%u%s (reboot to reconnect)\n", secure ? "wss" : "ws",
                    host.c_str(), port, path.c_str());
    } else {
      Serial.println("invalid WebSocket URL");
    }
    return;
  }

  Serial.println("unknown command; type HELP");
}

void loadConfig() {
  g_config.wifi_ssid = g_prefs.getString("wifi_ssid", config::kDefaultWifiSsid);
  g_config.wifi_password = g_prefs.getString("wifi_pass", config::kDefaultWifiPassword);
  g_config.ws_host = g_prefs.getString("ws_host", config::kDefaultWsHost);
  g_config.ws_port = g_prefs.getUShort("ws_port", config::kDefaultWsPort);
  g_config.ws_path = g_prefs.getString("ws_path", config::kDefaultWsPath);
  g_config.ws_secure = g_prefs.getBool("ws_secure", config::kDefaultWsSecure);
  g_config.modality = g_prefs.getString("modality", config::kDefaultModality);
  g_config.day_or_week = g_prefs.getUChar("day", config::kDefaultDayOrWeek);
  g_config.hx_offset = g_prefs.getInt("hx_offset", 0);
  g_config.hx_scale = g_prefs.getFloat("hx_scale", 0.0f);
}
}  // namespace

void provisioningBegin() {
  g_prefs.begin("syncfit", false);
  loadConfig();
  Serial.println("[provisioning] ready; type HELP");
}

void provisioningLoop() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n') {
      handleLine(g_line);
      g_line = "";
    } else if (c != '\r') {
      if (g_line.length() < 160) g_line += c;
    }
  }
}

const DeviceConfig& deviceConfig() {
  return g_config;
}

void provisioningSaveCalibration(std::int32_t offset, float scale) {
  g_config.hx_offset = offset;
  g_config.hx_scale = scale;
  g_prefs.putInt("hx_offset", offset);
  g_prefs.putFloat("hx_scale", scale);
  Serial.printf("[provisioning] calibration saved: offset=%ld scale=%.4f\n",
                static_cast<long>(offset), scale);
}

void provisioningConsumeTareRequest(bool& requested) {
  requested = g_tare_requested;
  g_tare_requested = false;
}

void provisioningConsumeCalibration(float& known_kg, bool& requested) {
  known_kg = g_cal_kg;
  requested = g_cal_requested;
  g_cal_requested = false;
}

}  // namespace syncfit
