# SyncFit Edge — Format A wiring

**Board:** ESP32 DevKit V1, 30-pin, USB-C integrated.  
**Logic / sensors:** 3.3 V.  
**Diagram:** [`definitive.svg`](definitive.svg) · [`definitive.png`](definitive.png)  
**BOM:** [`bom.csv`](bom.csv)

> The diagram labels are in Spanish for the printed build sheet. This document keeps the repository's English convention.

## 1. Power rails

| Rail | Source | Consumers |
|---|---|---|
| `+5V` | USB-C integrated | ESP32 DevKit only |
| `X` (`+3V3`) | ESP32 DevKit regulator | MAX30102, MLX90614, HX711 |
| `Y` (`GND`) | USB-C / DevKit | All modules and sensors |

The 400-point board has **one pair of rails, X and Y**. Do not feed `VIN` and
USB-C at the same time. The HX711 is powered from **3.3 V**, not 5 V, so
`DT`/`SCK` remain 3.3 V logic.

## 2. ESP32 30-pin header map

The board has 15 pins per side. This is the **generic 30-pin ESP32 DevKit
V1 with integrated USB-C**; the order below matches the labels printed on the
board in the reference photo (USB at the bottom, antenna at the top).

### Side A (left, pins 1–15, top to bottom)

| Pin | GPIO | Use in v1 | Notes |
|---:|---|---|---|
| 1 | `EN` | Reset | On-board reset button |
| 2 | `GPIO36` | Reserved | Input only (`VP`) |
| 3 | `GPIO39` | Reserved | Input only (`VN`) |
| 4 | `GPIO34` | HX711 DT / DOUT | Input only |
| 5 | `GPIO35` | Battery ADC | Input only; Format B divider |
| 6 | `GPIO32` | Reserved | Free |
| 7 | `GPIO33` | Reserved | Free |
| 8 | `GPIO25` | HX711 SCK | Output |
| 9 | `GPIO26` | Reserved | Free |
| 10 | `GPIO27` | Reserved | Free |
| 11 | `GPIO14` | Reserved | Strapping; leave free |
| 12 | `GPIO12` | Reserved | Strapping; leave free |
| 13 | `GPIO13` | Reserved | Free |
| 14 | `GND` | Ground | Common ground |
| 15 | `VIN` | +5 V input | USB-C powers the board; do not feed externally |

### Side B (right, pins 16–30, top to bottom)

| Pin | GPIO | Use in v1 | Notes |
|---:|---|---|---|
| 16 | `GPIO23` | MAX30102 INT | Input, active low |
| 17 | `GPIO22` | I2C SCL | 4.7 kΩ pull-up to 3V3 |
| 18 | `GPIO1` | UART TX0 | Programming / console |
| 19 | `GPIO3` | UART RX0 | Programming / console |
| 20 | `GPIO21` | I2C SDA | 4.7 kΩ pull-up to 3V3 |
| 21 | `GPIO19` | Reserved | Free |
| 22 | `GPIO18` | Status LED | Output, 330 Ω to LED to GND |
| 23 | `GPIO5` | Reserved | Strapping pin; leave free |
| 24 | `GPIO17` | Buzzer (optional) | Output through NPN transistor |
| 25 | `GPIO16` | Reserved | Free |
| 26 | `GPIO4` | Reserved | No tare button in the definitive build; leave free |
| 27 | `GPIO2` | Reserved | On-board LED / strapping |
| 28 | `GPIO15` | Reserved | Strapping pin; leave free |
| 29 | `GND` | Ground | Common ground |
| 30 | `3V3` | +3.3 V rail | Power for sensors |

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

### HX711 — two 3-wire half-bridge cells

| Module pin | ESP32 | Wire colour |
|---|---|---|
| `VCC` | `X` (`3V3`) | Red |
| `GND` | `Y` (`GND`) | Black |
| `DT` | `GPIO34` | Green |
| `SCK` | `GPIO25` | Green |

Use **two identical 3-wire cells**. Each cell is a half-bridge: the white wire is
the center; red and black are the ends.

| HX711 terminal | Cell wires |
|---|---|
| `E+` | Cell A red + Cell B red |
| `E−` | Cell A black + Cell B black |
| `A+` | Cell A white |
| `A−` | Cell B white |

If the reading goes negative when pressing, swap `A+` and `A−`, or rotate one
cell 180°. Join the two reds and two blacks with a small terminal block/Wago or a
soldered pigtail; the HX711 has only one hole per terminal.

HX711 at **10 SPS**, gain 128, channel A. Keep the cell wires short and twisted.
The final capacity depends on the mechanical mount: if the cells share the load,
it can approach 2× the single-cell rating; if one takes the full load, it stays
at the single-cell rating. Use a mechanical stop.

### LED

| Signal | ESP32 | Wiring |
|---|---|---|
| Status LED | `GPIO18` | LED anode (long) → column 7; LED cathode (short) → column 8; 330 Ω between 8b and 9a; 9b → Y with M-M |
| Buzzer (optional) | `GPIO17` | GPIO17 → 1 kΩ → 2N2222 base; buzzer between X and collector |

There is **no tare button** in the definitive build. Leave GPIO4 unconnected;
use the serial command `TARE`. Do not connect a bare LED without the 330 Ω
resistor (220 Ω–1 kΩ is acceptable).

### Battery monitor (Format B only)

`BAT_ADC` → `GPIO35`, using a 100 kΩ / 100 kΩ divider from LiPo+ to GND and 100 nF from GPIO35 to GND. Do not connect the battery divider in Format A.

## 4. Connectors

| Connector | Type | Pinout |
|---|---|---|
| J1 | USB-C integrated | 5 V, GND, D+, D− |
| J2 | JST-XH 5-pin | 1: 3V3 · 2: GND · 3: SDA · 4: SCL · 5: INT |
| J3 | JST-XH 4-pin | 1: 3V3 · 2: GND · 3: SDA · 4: SCL |
| J4 | JST-XH 4-pin | 1: 3V3 · 2: GND · 3: DT · 4: SCK |
| J5 | 4-pin screw terminal | E+, E−, A+, A− (two 3-wire cells) |
| J6 | 2-pin header (optional) | GPIO4, GND — not used without a tare button |
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
4. HX711: connect the two 3-wire cells to a terminal block/Wago, then to `E+`,
   `E−`, `A+`, `A−`. Run `TARE` and `CAL <kg>` from the serial monitor.
5. Verify the LED with the 330 Ω resistor before connecting it.
6. Check that the two cells are mounted so they flex in opposite directions.
7. Send telemetry to the local backend WebSocket, not production, until the hardware is validated.
