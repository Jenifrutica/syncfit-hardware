#include "status_led.hpp"

namespace syncfit {

void StatusLed::begin() {
  pinMode(pin_, OUTPUT);
  digitalWrite(pin_, LOW);
}

void StatusLed::update(std::uint32_t now_ms) {
  if (error_) {
    digitalWrite(pin_, (now_ms % 1200) < 120 ? HIGH : LOW);
    return;
  }
  if (cloud_) {
    digitalWrite(pin_, HIGH);
    return;
  }
  if (wifi_) {
    digitalWrite(pin_, (now_ms % 1000) < 100 ? HIGH : LOW);
    return;
  }
  digitalWrite(pin_, (now_ms % 2000) < 80 ? HIGH : LOW);
}

}  // namespace syncfit
