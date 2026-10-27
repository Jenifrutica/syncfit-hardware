#include "drivers/hx711.hpp"

namespace syncfit {

void Hx711::begin() {
  pinMode(dout_pin_, INPUT);
  pinMode(sck_pin_, OUTPUT);
  digitalWrite(sck_pin_, LOW);
}

bool Hx711::isReady() const {
  return digitalRead(dout_pin_) == LOW;
}

bool Hx711::readRaw(std::int32_t& value) {
  if (!isReady()) return false;

  std::uint32_t raw = 0;
  for (int i = 0; i < 24; ++i) {
    digitalWrite(sck_pin_, HIGH);
    delayMicroseconds(1);
    raw = (raw << 1) | (digitalRead(dout_pin_) == HIGH ? 1u : 0u);
    digitalWrite(sck_pin_, LOW);
    delayMicroseconds(1);
  }

  // 25th pulse selects channel A, gain 128 for the next conversion.
  digitalWrite(sck_pin_, HIGH);
  delayMicroseconds(1);
  digitalWrite(sck_pin_, LOW);
  delayMicroseconds(1);

  if (raw & 0x800000u) raw |= 0xFF000000u;
  value = static_cast<std::int32_t>(raw);
  return true;
}

bool Hx711::readAverage(std::uint8_t samples, std::int32_t& value) {
  if (samples == 0) return false;
  std::int64_t sum = 0;
  std::uint8_t read = 0;
  const std::uint32_t start = millis();
  while (read < samples) {
    std::int32_t raw = 0;
    if (readRaw(raw)) {
      sum += raw;
      ++read;
    } else {
      if (millis() - start > 2000) return false;
      delay(1);
    }
  }
  value = static_cast<std::int32_t>(sum / samples);
  return true;
}

bool Hx711::tare(std::uint8_t samples) {
  std::int32_t average = 0;
  if (!readAverage(samples, average)) return false;
  offset_ = average;
  return true;
}

bool Hx711::calibrate(float known_kg, std::uint8_t samples) {
  if (known_kg <= 0.0f) return false;
  std::int32_t average = 0;
  if (!readAverage(samples, average)) return false;
  const float delta = static_cast<float>(average - offset_);
  if (delta == 0.0f) return false;
  scale_ = delta / known_kg;
  return true;
}

bool Hx711::readKg(float& kg) {
  std::int32_t raw = 0;
  if (!readRaw(raw)) return false;
  if (scale_ == 0.0f) return false;
  kg = static_cast<float>(raw - offset_) / scale_;
  return true;
}

}  // namespace syncfit
