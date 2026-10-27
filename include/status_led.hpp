// Status LED patterns: boot, Wi-Fi, cloud connection and error.

#pragma once

#include <Arduino.h>

#include <cstdint>

namespace syncfit {

class StatusLed {
 public:
  explicit StatusLed(std::uint8_t pin) : pin_(pin) {}

  void begin();
  void setWifi(bool connected) { wifi_ = connected; }
  void setCloud(bool connected) { cloud_ = connected; }
  void setError(bool error) { error_ = error; }
  void update(std::uint32_t now_ms);

 private:
  std::uint8_t pin_;
  bool wifi_ = false;
  bool cloud_ = false;
  bool error_ = false;
};

}  // namespace syncfit
