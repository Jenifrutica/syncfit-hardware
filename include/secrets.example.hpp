// Copy to `secrets.hpp` for local builds. Never commit `secrets.hpp`.
// These are compile-time defaults; runtime commands can override Wi-Fi and WS
// settings and persist them in NVS.

#pragma once

#define SYNCFIT_WIFI_SSID "your-ssid"
#define SYNCFIT_WIFI_PASSWORD "your-password"
#define SYNCFIT_WS_HOST "192.168.1.100"
#define SYNCFIT_WS_PORT 8000
#define SYNCFIT_WS_PATH "/api/v1/ws/telemetry"
#define SYNCFIT_WS_SECURE 0
