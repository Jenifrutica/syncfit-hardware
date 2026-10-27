// SyncFit Edge — ESP32 DevKit V1 (30-pin, USB-C) pin map.
// Keep every hardware connection in this file; drivers and tasks must not
// contain magic pin numbers.

#pragma once

#include <cstdint>

namespace syncfit::pins {

// I2C bus (MAX30102 + MLX90614).
constexpr std::uint8_t kI2cSda = 21;
constexpr std::uint8_t kI2cScl = 22;
constexpr std::uint32_t kI2cClockHz = 400000;

// MAX30102 interrupt (active low, optional; FIFO polling is also supported).
constexpr std::uint8_t kMax30102Int = 23;

// HX711 load-cell amplifier.
constexpr std::uint8_t kHx711Dout = 34;  // input-only
constexpr std::uint8_t kHx711Sck = 25;

// User controls and status.
constexpr std::uint8_t kTareButton = 4;
constexpr std::uint8_t kStatusLed = 18;
constexpr std::uint8_t kBuzzer = 17;  // optional

// Battery monitor, Format B only.
constexpr std::uint8_t kBatteryAdc = 35;  // input-only, ADC1_CH7

// UART0 is the USB serial console.
constexpr std::uint8_t kUartTx = 1;
constexpr std::uint8_t kUartRx = 3;

// Analog reference and the 100k/100k battery divider.
constexpr float kAdcReferenceV = 3.3f;
constexpr float kBatteryDividerRatio = 2.0f;

}  // namespace syncfit::pins
