#include "net/ws_client.hpp"

namespace syncfit {

void WsClient::begin(const char* host, std::uint16_t port, const char* path, bool secure) {
  socket_.onEvent([this](WStype_t type, std::uint8_t* payload, std::size_t length) {
    handleEvent(type, payload, length);
  });
  socket_.setReconnectInterval(5000);
  socket_.enableHeartbeat(15000, 3000, 2);

  if (secure) {
    socket_.beginSSL(host, port, path);
  } else {
    socket_.begin(host, port, path);
  }
  Serial.printf("[WS] %s://%s:%u%s\n", secure ? "wss" : "ws", host, port, path);
}

void WsClient::loop() {
  socket_.loop();
}

bool WsClient::send(const char* payload) {
  if (!connected_ || payload == nullptr) return false;
  return socket_.sendTXT(payload);
}

void WsClient::handleEvent(WStype_t type, std::uint8_t* payload, std::size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      connected_ = true;
      Serial.println("[WS] connected");
      break;
    case WStype_DISCONNECTED:
      connected_ = false;
      Serial.println("[WS] disconnected");
      break;
    case WStype_TEXT:
      if (payload != nullptr && length > 0) {
        if (handler_ != nullptr) {
          handler_(reinterpret_cast<const char*>(payload));
        } else {
          Serial.printf("[WS] %.*s\n", static_cast<int>(length),
                        reinterpret_cast<const char*>(payload));
        }
      }
      break;
    case WStype_ERROR:
      Serial.println("[WS] error");
      break;
    default:
      break;
  }
}

}  // namespace syncfit
