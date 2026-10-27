# Drivers

Sensor drivers for the Edge-IoT device.

- **`max30102`** — optical PPG at 100 Hz (I2C `0x57`).
- **`mlx90614`** — infrared thermal (I2C `0x5A`, 3.3 V).
- **`hx711`** — load cell + ADC for isometric dynamometry (10 SPS).

Each driver exposes a small `read()` API and feeds the Ring Buffer in
`include/ring_buffer.hpp`. Nothing here does DSP or networking.

Implementation lives in `../src/drivers/`, headers in
`../include/drivers/`. The wiring and pinout are in `../hardware/`.
