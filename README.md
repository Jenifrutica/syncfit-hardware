# SyncFit Hardware

Physical sensing layer and embedded firmware of SyncFit Edge. Captures biomechanical telemetry at the source and streams it over WebSocket.

## Purpose

Turn the body into data at the edge: optical pulse, skin thermal variation and isometric strength, sampled on-device and transmitted as compact JSON frames at low latency.

## What belongs here

- **Firmware** (`firmware/`): ESP32 using ESP-IDF / Arduino Core, I2C and SPI buses.
- **Sensor drivers** (`drivers/`):
  - MAX30102 optical PPG at 100 Hz.
  - MLX90614 infrared thermal sensor.
  - HX711 load-cell + ADC for isometric dynamometry.
- **On-device processing** (`include/`, `lib/`): fixed-window Ring Buffer, basic filtering, frame validation.
- **Hardware design** (`hardware/`): wiring diagrams, pinout, schematic and bill of materials (BOM).
- **Form factors**: *Format A* fixed check-in station (mains/USB, maximum optical stability) and *Format B* wearable wrist band (LiPo battery, continuous warm-up monitoring).

## What does NOT belong here

- DSP beyond basic filtering, ML models or AI reasoning.
- REST business logic, persistence or UI.

## Data Structures

| Structure | Complexity | Purpose |
|-----------|:----------:|---------|
| **Ring Buffer** | O(1) insert | Mandatory: continuous 100 Hz PPG stream in fixed static memory, avoiding RAM overload and heap fragmentation. |
| **Lock-free SPSC queue** | O(1) | Recommended: ISR-to-task hand-off. |
| **Fixed-capacity max-heap** | O(log n) | Recommended: on-device alert prioritization before transmitting. |

The Ring Buffer is explicitly required by the technical document, which specifies it in both C++ and Python.

## Suggested structure

```
syncfit-hardware/
├── firmware/           # application entry point (ESP-IDF / Arduino)
├── drivers/            # max30102, mlx90614, hx711
├── include/
│   └── ring_buffer.hpp
├── lib/
├── hardware/           # schematic, pinout, BOM
├── platformio.ini
└── README.md
```

## Stack

C++ (ESP-IDF / Arduino Core), I2C / SPI, WebSocket client over Wi-Fi.

## Tasks

> **Language: C++ (ESP-IDF / Arduino Core).** C++ is mandatory for the ESP32 firmware.

### Requirements

- [ ] Set up the ESP-IDF / Arduino project and build system.
- [ ] Implement the MAX30102 optical PPG driver at 100 Hz.
- [ ] Implement the MLX90614 infrared thermal driver.
- [ ] Implement the HX711 load-cell driver for isometric dynamometry.
- [ ] Implement the **Ring Buffer** with O(1) insertion in fixed static memory.
- [ ] Implement the lock-free SPSC queue (ISR to task).
- [ ] Implement the fixed-capacity max-heap for on-device alert prioritization.
- [ ] Implement basic filtering and frame validation.
- [ ] Implement the WebSocket client over Wi-Fi.
- [ ] Validate emitted frames against `syncfit-contracts`.
- [ ] Produce Format A artifacts: wiring diagram, pinout, schematic and BOM.
- [ ] Produce Format B wearable design (enclosure and LiPo power).

## Related repositories

- [`syncfit-contracts`](../syncfit-contracts) — telemetry frame schema.
- [`syncfit-backend`](../syncfit-backend) — WebSocket ingestion endpoint.
- [`syncfit-simulator`](../syncfit-simulator) — replaces this layer during software development.

All code, comments, documentation and commits in this repository are written in English.

## Handoff for the team

**Role.** Physical sensing + firmware (ESP32, MAX30102 PPG, MLX90614, HX711).

**Run / test.** `pio run` · `pio test -e native`.

**State.** Implemented: `include/ring_buffer.hpp` (O(1) fixed-memory buffer,
host-tested). Scaffold: `src/main.cpp` simulates sampling; TODO real drivers,
telemetry frame build and the WebSocket client. See `hardware/README.md` for the
two form factors (totem / wearable).

## Context for a new session

**What it is.** ESP32 firmware + sensing layer (skeleton). Emits contract-valid
telemetry. Replaced by syncfit-simulator until built.

**Stack.** C++ (ESP-IDF / Arduino Core), PlatformIO. Sensors: MAX30102 (PPG
100 Hz, I2C), MLX90614 (thermal), HX711 (load cell).

**Layout.** `platformio.ini` (esp32dev + native test env), `src/main.cpp`,
`include/ring_buffer.hpp` (mandatory O(1) Ring Buffer), `drivers/`, `hardware/`
(wiring/pinout/BOM, Formats A totem / B wearable), `test/` (Unity, host).

**Goal.** Emit `TelemetryFrame` JSON over WebSocket to the backend; real captures
drop into `syncfit-simulator/captures/`.

**Run.** `pio run` / `pio test -e native`.
