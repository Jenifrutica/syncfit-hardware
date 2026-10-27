#include "drivers/max30102.hpp"

namespace syncfit {
namespace {
// MAX30102 register map (datasheet).
constexpr std::uint8_t kRegInterruptStatus = 0x00;
constexpr std::uint8_t kRegInterruptEnable = 0x01;
constexpr std::uint8_t kRegFifoWritePtr = 0x04;
constexpr std::uint8_t kRegOverflowCounter = 0x05;
constexpr std::uint8_t kRegFifoReadPtr = 0x06;
constexpr std::uint8_t kRegFifoData = 0x07;
constexpr std::uint8_t kRegFifoConfig = 0x08;
constexpr std::uint8_t kRegModeConfig = 0x09;
constexpr std::uint8_t kRegSpO2Config = 0x0A;
constexpr std::uint8_t kRegLed1Pa = 0x0C;
constexpr std::uint8_t kRegLed2Pa = 0x0D;
constexpr std::uint8_t kRegPartId = 0xFF;

constexpr std::uint8_t kModeReset = 0x40;
constexpr std::uint8_t kModeSpO2 = 0x03;
constexpr std::uint8_t kFifoRolloverAlmostFull = 0x1F;
// ADC range 4096 nA, 100 Hz, 411 us pulse width.
constexpr std::uint8_t kSpO2Config100Hz = 0x27;
constexpr std::uint8_t kLedCurrent = 0x24;  // ~7.2 mA

constexpr std::uint8_t kPartIdMax30102 = 0x15;
constexpr std::size_t kFifoDepth = 32;
}  // namespace

bool Max30102::begin(TwoWire& wire) {
  wire_ = &wire;

  std::uint8_t part = 0;
  if (readReg(kRegPartId, part) && part != kPartIdMax30102) {
    Serial.printf("[MAX30102] unexpected part id 0x%02X\n", part);
    return false;
  }

  if (!writeReg(kRegModeConfig, kModeReset)) return false;
  delay(100);
  for (int i = 0; i < 20; ++i) {
    std::uint8_t mode = 0;
    if (readReg(kRegModeConfig, mode) && (mode & kModeReset) == 0) break;
    delay(10);
  }

  if (!writeReg(kRegFifoConfig, kFifoRolloverAlmostFull)) return false;
  if (!writeReg(kRegModeConfig, kModeSpO2)) return false;
  if (!writeReg(kRegSpO2Config, kSpO2Config100Hz)) return false;
  if (!writeReg(kRegLed1Pa, kLedCurrent)) return false;
  if (!writeReg(kRegLed2Pa, kLedCurrent)) return false;
  if (!writeReg(kRegInterruptEnable, 0x40)) return false;  // FIFO almost full
  if (!writeReg(kRegInterruptStatus, 0x00)) return false;

  writeReg(kRegFifoWritePtr, 0x00);
  writeReg(kRegOverflowCounter, 0x00);
  writeReg(kRegFifoReadPtr, 0x00);
  return true;
}

bool Max30102::present() {
  std::uint8_t part = 0;
  return readReg(kRegPartId, part) && part == kPartIdMax30102;
}

bool Max30102::readFifo(float* ir_samples, float* red_samples, std::size_t max_samples,
                        std::size_t& count) {
  count = 0;
  if (wire_ == nullptr || ir_samples == nullptr || red_samples == nullptr ||
      max_samples == 0) {
    return false;
  }

  std::uint8_t write_ptr = 0;
  std::uint8_t read_ptr = 0;
  if (!readReg(kRegFifoWritePtr, write_ptr) || !readReg(kRegFifoReadPtr, read_ptr)) {
    return false;
  }

  std::size_t available = (write_ptr + kFifoDepth - read_ptr) % kFifoDepth;
  if (available == 0) return false;
  if (available > max_samples) available = max_samples;

  std::uint8_t raw[6 * kFifoDepth] = {};
  if (!readBytes(kRegFifoData, raw, available * 6)) return false;

  for (std::size_t i = 0; i < available; ++i) {
    const std::uint8_t* sample = raw + i * 6;
    const std::uint32_t red =
        (static_cast<std::uint32_t>(sample[0]) << 16 |
         static_cast<std::uint32_t>(sample[1]) << 8 | sample[2]) &
        0x3FFFFu;
    const std::uint32_t ir =
        (static_cast<std::uint32_t>(sample[3]) << 16 |
         static_cast<std::uint32_t>(sample[4]) << 8 | sample[5]) &
        0x3FFFFu;
    red_samples[i] = static_cast<float>(red);
    ir_samples[i] = static_cast<float>(ir);
  }

  count = available;
  return true;
}

bool Max30102::writeReg(std::uint8_t reg, std::uint8_t value) {
  if (wire_ == nullptr) return false;
  wire_->beginTransmission(address_);
  wire_->write(reg);
  wire_->write(value);
  return wire_->endTransmission() == 0;
}

bool Max30102::readReg(std::uint8_t reg, std::uint8_t& value) {
  return readBytes(reg, &value, 1);
}

bool Max30102::readBytes(std::uint8_t reg, std::uint8_t* data, std::size_t length) {
  if (wire_ == nullptr || data == nullptr || length == 0) return false;
  wire_->beginTransmission(address_);
  wire_->write(reg);
  if (wire_->endTransmission(false) != 0) return false;
  if (wire_->requestFrom(static_cast<int>(address_), static_cast<int>(length)) !=
      static_cast<int>(length)) {
    return false;
  }
  for (std::size_t i = 0; i < length && wire_->available(); ++i) {
    data[i] = static_cast<std::uint8_t>(wire_->read());
  }
  return true;
}

}  // namespace syncfit
