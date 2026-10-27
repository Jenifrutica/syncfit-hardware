// HX711 24-bit load-cell amplifier. Bit-banged two-wire interface.

#pragma once

#include <Arduino.h>

#include <cstdint>

namespace syncfit {

class Hx711 {
 public:
  Hx711(std::uint8_t dout_pin, std::uint8_t sck_pin)
      : dout_pin_(dout_pin), sck_pin_(sck_pin) {}

  void begin();
  bool isReady() const;
  bool readRaw(std::int32_t& value);
  bool readAverage(std::uint8_t samples, std::int32_t& value);
  bool tare(std::uint8_t samples = 10);
  bool calibrate(float known_kg, std::uint8_t samples = 10);

  void setOffset(std::int32_t offset) { offset_ = offset; }
  void setScale(float scale) { scale_ = scale; }
  std::int32_t offset() const { return offset_; }
  float scale() const { return scale_; }
  bool readKg(float& kg);

 private:
  std::uint8_t dout_pin_;
  std::uint8_t sck_pin_;
  std::int32_t offset_ = 0;
  float scale_ = 0.0f;  // 0 = not calibrated; readKg() returns false
};

}  // namespace syncfit
