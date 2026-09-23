# Drivers

Sensor drivers for the Edge-IoT device. To be implemented:

- **`max30102`** — optical PPG at 100 Hz (I2C).
- **`mlx90614`** — infrared thermal sensor (I2C).
- **`hx711`** — load cell + ADC for isometric dynamometry.

Each driver exposes a small `read()` API and feeds the Ring Buffer in
`include/ring_buffer.hpp`. Nothing here does DSP or networking.
