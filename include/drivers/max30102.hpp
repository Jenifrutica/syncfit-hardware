// MAX30102 optical PPG driver. Register-level I2C, no external sensor library.

#pragma once

#include <Arduino.h>
#include <Wire.h>

#include <cstddef>
#include <cstdint>

namespace syncfit {

class Max30102 {
 public:
  explicit Max30102(std::uint8_t address = 0x57) : address_(address) {}

  bool begin(TwoWire& wire = Wire);
  bool present();
  bool readFifo(float* ir_samples, float* red_samples, std::size_t max_samples,
                std::size_t& count);

 private:
  bool writeReg(std::uint8_t reg, std::uint8_t value);
  bool readReg(std::uint8_t reg, std::uint8_t& value);
  bool readBytes(std::uint8_t reg, std::uint8_t* data, std::size_t length);

  TwoWire* wire_ = nullptr;
  std::uint8_t address_;
};

}  // namespace syncfit
