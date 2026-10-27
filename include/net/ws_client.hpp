// WebSocket client wrapper for the telemetry endpoint.

#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>

#include <cstddef>
#include <cstdint>

namespace syncfit {

class WsClient {
 public:
  using MessageHandler = void (*)(const char* message);

  void begin(const char* host, std::uint16_t port, const char* path, bool secure);
  void setMessageHandler(MessageHandler handler) { handler_ = handler; }
  void loop();
  bool connected() const { return connected_; }
  bool send(const char* payload);

 private:
  void handleEvent(WStype_t type, std::uint8_t* payload, std::size_t length);

  WebSocketsClient socket_;
  MessageHandler handler_ = nullptr;
  bool connected_ = false;
};

}  // namespace syncfit
