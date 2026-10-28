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

- [x] Set up the ESP-IDF / Arduino project and build system.
- [x] Implement the MAX30102 optical PPG driver at 100 Hz.
- [x] Implement the MLX90614 infrared thermal driver.
- [x] Implement the HX711 load-cell driver for isometric dynamometry.
- [x] Implement the **Ring Buffer** with O(1) insertion in fixed static memory.
- [x] Implement the lock-free SPSC queue (ISR to task).
- [x] Implement the fixed-capacity max-heap for on-device alert prioritization.
- [x] Implement basic filtering and frame validation.
- [x] Implement the WebSocket client over Wi-Fi.
- [x] Validate emitted frames against `syncfit-contracts` (manual).
- [ ] Add contract validation to CI.
- [x] Produce Format A wiring diagram, pinout and BOM.
- [x] Produce the solderless protoboard layout and jumper list.
- [ ] Produce the electrical schematic.
- [ ] Produce Format B wearable design (enclosure and LiPo power).

## Related repositories

- [`syncfit-contracts`](../syncfit-contracts) — telemetry frame schema.
- [`syncfit-backend`](../syncfit-backend) — WebSocket ingestion endpoint.
- [`syncfit-simulator`](../syncfit-simulator) — replaces this layer during software development.

All code, comments, documentation and commits in this repository are written in English.

## Handoff for the team

**Role.** Physical sensing + firmware (ESP32, MAX30102 PPG, MLX90614, HX711).

**Run / test.**

```bash
pio test -e native              # host tests: filters, frame, queues, heap
pio run -e esp32dev             # real sensors
pio run -e esp32dev_mock        # synthetic sensors, no wiring needed
pio run -e esp32dev_mock -t upload
pio device monitor -b 115200
```

Copy `include/secrets.example.hpp` to `include/secrets.hpp` for local
credentials (the real file is git-ignored). Runtime provisioning also works over
serial: `HELP`, `STATUS`, `SET MODALITY GESTATIONAL`, `SET WEEK 16`,
`SET WIFI <ssid>,<password>`,
`SET WS ws://<laptop-ip>:8000/api/v1/ws/telemetry`, `TARE`, `CAL <kg>`.

**State.** Implemented: ring buffer, SPSC queue, max-heap, filters, contract
telemetry frame, MAX30102/MLX90614/HX711 drivers, FreeRTOS tasks, serial/NVS
provisioning, Wi-Fi WebSocket client, mock-sensor build and Format A build sheet
(`hardware/wiring.svg`, `wiring.png`, `pinout.md`, `bom.csv`). Pending:
hardware-in-the-loop calibration, CI contract validation, electrical schematic
and Format B. Format A also has a solderless protoboard layout
(`hardware/protoboard.svg`) with a per-net jumper list. See
`hardware/README.md` for both form factors.

## Context for a new session

**What it is.** ESP32 firmware + sensing layer. Emits contract-valid telemetry
over WebSocket. Can run without sensors using `esp32dev_mock`.

**Stack.** C++ (Arduino Core), PlatformIO. Sensors: MAX30102 (PPG 100 Hz, I2C),
MLX90614 (thermal, 1 Hz), HX711 (load cell, 10 SPS). Wi-Fi WebSocket client.

**Layout.** `platformio.ini` (`esp32dev`, `esp32dev_mock`, `native`),
`src/main.cpp` (tasks), `src/drivers/`, `include/` (pins, config, filters, frame,
queues), `test/` (Unity, host), `hardware/` (wiring, pinout, BOM).

**Goal.** Emit `TelemetryFrame` JSON over WebSocket to the backend. Real captures
drop into `syncfit-simulator/captures/`.

**Run.** `pio run -e esp32dev_mock` for a sensorless smoke test; `pio run -e
esp32dev` after wiring; `pio test -e native` for host tests.

## Roadmap · Qué falta (español)

> Estado: **firmware v0.1 listo para banco**. Compila para ESP32, pasa pruebas
> nativas y tiene modo mock. Falta validarlo con los sensores reales.

- **Calibración real**: TARE y `CAL <kg>` con la celda de 50 kg.
- **Prueba I2C**: confirmar `0x57` (MAX30102) y `0x5A` (MLX90614).
- **WebSocket local**: probar contra `ws://<laptop>:8000/api/v1/ws/telemetry`.
- **Validación de contrato** automática en CI.
- **Esquemático eléctrico** y **Format B** (banda con LiPo).
- **CI** (`pio test`).
