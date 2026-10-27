// MLX90614 infrared thermometer (3.3 V variant), I2C 0x5A.

#pragma once

#include <Arduino.h>
#include <Wire.h>

#include <cstdint>

namespace syncfit {

class Mlx90614 {
 public:
  explicit Mlx90614(std::uint8_t address = 0x5A) : address_(address) {}

  bool begin(TwoWire& wire = Wire);
  bool readObjectC(float& temperature_c);
  bool readAmbientC(float& temperature_c);

 private:
  bool readTemperature(std::uint8_t register_address, float& temperature_c);

  TwoWire* wire_ = nullptr;
  std::uint8_t address_;
};

}  // namespace syncfit
