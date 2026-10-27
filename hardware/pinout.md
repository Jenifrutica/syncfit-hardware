# SyncFit Edge — Format A wiring

**Board:** ESP32 DevKit V1, 30-pin, USB-C integrated.  
**Logic / sensors:** 3.3 V.  
**Diagram:** [`wiring.svg`](wiring.svg) · [`wiring.png`](wiring.png)  
**BOM:** [`bom.csv`](bom.csv)

> The diagram labels are in Spanish for the printed build sheet. This document keeps the repository's English convention.

## 1. Power rails

| Rail | Source | Consumers |
|---|---|---|
| `+5V` | USB-C integrated | ESP32 DevKit only |
| `+3V3` | ESP32 DevKit regulator | MAX30102, MLX90614, HX711 |
| `GND` | USB-C / DevKit | All modules and sensors |

Do not feed `VIN` and USB-C at the same time. The HX711 is powered from **3.3 V**, not 5 V, so `DT`/`SCK` remain 3.3 V logic.

## 2. ESP32 30-pin header map

The board has 15 pins per side. Pin numbers below follow the physical order used in the diagram.

### Side A (pins 1–15)

| Pin | GPIO | Use in v1 | Notes |
|---:|---|---|---|
| 1 | `3V3` | +3.3 V rail | Power for sensors |
| 2 | `GND` | Ground rail | Common ground |
| 3 | `GPIO15` | Reserved | Strapping pin; leave free |
| 4 | `GPIO2` | Reserved | On-board LED / strapping |
| 5 | `GPIO4` | Tare button | Input, internal pull-up, button to GND |
| 6 | `GPIO16` | Reserved | Free |
| 7 | `GPIO17` | Buzzer (optional) | Output through NPN transistor |
| 8 | `GPIO5` | Reserved | Strapping pin; leave free |
| 9 | `GPIO18` | Status LED | Output, 330 Ω to LED to GND |
| 10 | `GPIO19` | Reserved | Free |
| 11 | `GPIO21` | I2C SDA | 4.7 kΩ pull-up to 3V3 |
| 12 | `GPIO3` | UART RX | Programming / console |
| 13 | `GPIO1` | UART TX | Programming / console |
| 14 | `GPIO22` | I2C SCL | 4.7 kΩ pull-up to 3V3 |
| 15 | `GPIO23` | MAX30102 INT | Input, active low |

### Side B (pins 16–30)

| Pin | GPIO | Use in v1 | Notes |
|---:|---|---|---|
| 16 | `EN` | Reset | On-board reset button |
| 17 | `GPIO36` | Reserved | Input only |
| 18 | `GPIO39` | Reserved | Input only |
| 19 | `GPIO34` | HX711 DT / DOUT | Input only |
| 20 | `GPIO35` | Battery ADC | Input only; Format B divider |
| 21 | `GPIO32` | Reserved | Free |
| 22 | `GPIO33` | Reserved | Free |
| 23 | `GPIO25` | HX711 SCK | Output |
| 24 | `GPIO26` | Reserved | Free |
| 25 | `GPIO27` | Reserved | Free |
| 26 | `GPIO14` | Reserved | Strapping; leave free |
| 27 | `GPIO12` | Reserved | Strapping; leave free |
| 28 | `GPIO13` | Reserved | Free |
| 29 | `GND` | Ground | Second ground pin |
| 30 | `VIN` | +5 V input | Reserved; USB-C powers the board |

## 3. Module connections

### MAX30102 — PPG

| Module pin | ESP32 | Wire colour |
|---|---|---|
| `VIN` | `3V3` | Red |
| `GND` | `GND` | Black |
| `SDA` | `GPIO21` | Blue |
| `SCL` | `GPIO22` | Cyan |
| `INT` | `GPIO23` | Purple |

I2C address: **0x57**. Sample rate: **100 Hz**. Use 3.3 V only.

### MLX90614 — infrared temperature

| Module pin | ESP32 | Wire colour |
|---|---|---|
| `VIN` | `3V3` | Red |
| `GND` | `GND` | Black |
| `SDA` | `GPIO21` | Blue |
| `SCL` | `GPIO22` | Cyan |

I2C address: **0x5A**. Use the **3.3 V** variant. Read object and ambient temperature.

### HX711 — 50 kg load cell

| Module pin | ESP32 | Wire colour |
|---|---|---|
| `VCC` | `3V3` | Red |
| `GND` | `GND` | Black |
| `DT` | `GPIO34` | Green |
| `SCK` | `GPIO25` | Green |

Load-cell terminal:

| Terminal | Load-cell wire |
|---|---|
| `E+` | Red |
| `E-` | Black |
| `A+` | Green |
| `A-` | White |

HX711 at **10 SPS**, gain 128, channel A. Keep the load-cell wires short and twisted; connect the shield to GND on the HX711 side only. The 50 kg cell is the rated limit: use a mechanical stop and do not perform maximum grip tests without it.

### Button and LED

| Signal | ESP32 | Wiring |
|---|---|---|
| Tare button | `GPIO4` | Button between GPIO4 and GND; internal pull-up |
| Status LED | `GPIO18` | GPIO18 → 330 Ω → LED anode → LED cathode → GND |
| Buzzer (optional) | `GPIO17` | GPIO17 → 1 kΩ → 2N2222 base; buzzer between +3V3 and collector |

### Battery monitor (Format B only)

`BAT_ADC` → `GPIO35`, using a 100 kΩ / 100 kΩ divider from LiPo+ to GND and 100 nF from GPIO35 to GND. Do not connect the battery divider in Format A.

## 4. Connectors

| Connector | Type | Pinout |
|---|---|---|
| J1 | USB-C integrated | 5 V, GND, D+, D− |
| J2 | JST-XH 5-pin | 1: 3V3 · 2: GND · 3: SDA · 4: SCL · 5: INT |
| J3 | JST-XH 4-pin | 1: 3V3 · 2: GND · 3: SDA · 4: SCL |
| J4 | JST-XH 4-pin | 1: 3V3 · 2: GND · 3: DT · 4: SCK |
| J5 | 4-pin screw terminal | E+, E−, A+, A− |
| J6 | JST-XH 2-pin | GPIO4, GND |
| J7 | JST-PH 2-pin | BAT+, GND (Format B) |
| J8 | JST-XH 2-pin | GPIO18, GND |
| J9 | JST-XH 2-pin | GPIO17, GND (optional buzzer) |
| J10 | 4-pin header | 3V3, GND, TX, RX |

## 5. Test points

`TP1 3V3`, `TP2 5V`, `TP3 GND`, `TP4 SDA`, `TP5 SCL`, `TP6 HX_DT`, `TP7 HX_SCK`, `TP8 BAT_ADC`, `TP9 MAX_INT`, `TP10 LED`.

## 6. Bring-up checks

1. Continuity: 3V3 and GND must not short.
2. Power the board from USB-C only; measure 3.3 V on the sensor rail.
3. I2C scan: expect `0x57` (MAX30102) and `0x5A` (MLX90614).
4. HX711: read raw values with no load, then with known weights; run `TARE` and `CAL <kg>`.
5. Verify the LED and tare button before connecting the load cell.
6. Connect the load cell only after the HX711 readings are stable.
7. Send telemetry to the local backend WebSocket, not production, until the hardware is validated.
