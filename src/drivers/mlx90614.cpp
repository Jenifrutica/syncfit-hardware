#include "drivers/mlx90614.hpp"

namespace syncfit {
namespace {
constexpr std::uint8_t kRegAmbient = 0x06;
constexpr std::uint8_t kRegObject1 = 0x07;
constexpr float kKelvinOffset = 273.15f;
constexpr float kResolutionK = 0.02f;
}  // namespace

bool Mlx90614::begin(TwoWire& wire) {
  wire_ = &wire;
  float temperature = 0.0f;
  return readAmbientC(temperature);
}

bool Mlx90614::readObjectC(float& temperature_c) {
  return readTemperature(kRegObject1, temperature_c);
}

bool Mlx90614::readAmbientC(float& temperature_c) {
  return readTemperature(kRegAmbient, temperature_c);
}

bool Mlx90614::readTemperature(std::uint8_t register_address, float& temperature_c) {
  if (wire_ == nullptr) return false;
  wire_->beginTransmission(address_);
  wire_->write(register_address);
  if (wire_->endTransmission(false) != 0) return false;
  if (wire_->requestFrom(static_cast<int>(address_), 3) != 3) return false;

  const std::uint8_t lsb = static_cast<std::uint8_t>(wire_->read());
  const std::uint8_t msb = static_cast<std::uint8_t>(wire_->read());
  if (wire_->available()) wire_->read();  // PEC, not verified in v1

  const std::uint16_t raw = (static_cast<std::uint16_t>(msb) << 8) | lsb;
  temperature_c = static_cast<float>(raw) * kResolutionK - kKelvinOffset;
  return true;
}

}  // namespace syncfit
