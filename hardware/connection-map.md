# SyncFit Edge — exact connection map (400-point breadboard)

**Board:** ESP32 DevKit V1, generic 30-pin, USB-C.  
**Breadboard:** 400 points, used only as a bus. The ESP32 and the sensor modules
stay **free** beside it.  
**Diagram:** [`protoboard-columns.svg`](protoboard-columns.svg) · [`protoboard-columns.png`](protoboard-columns.png)

## 1. How the breadboard is wired

On a standard breadboard, the **five holes in one column** (a–e above the center
channel, f–j below) are connected internally. The **rows are not connected**.
The two long power rails are connected along their length.

That is why this map uses **one column per signal**:

| Breadboard column | Net | What connects there |
|---:|---|---|
| **1** | `SDA` | ESP32 `D21` + MAX30102 `SDA` + MLX90614 `SDA` |
| **2** | `SCL` | ESP32 `D22` + MAX30102 `SCL` + MLX90614 `SCL` |
| **3** | `D23 / INT` | ESP32 `D23` + MAX30102 `INT` |
| **4** | `D34 / HX_DT` | ESP32 `D34` + HX711 `DT` |
| **5** | `D25 / HX_SCK` | ESP32 `D25` + HX711 `SCK` |
| **6** | `D4 / TARE` | ESP32 `D4` + button leg 1 |
| **7** | `D18 / LED` | ESP32 `D18` + LED anode |
| **8** | `LED−` | LED cathode + 330 Ω leg 1 |
| **9** | `GND*` | 330 Ω leg 2 → M-M to the black `GND` rail |
| **Red rail** | `+3V3` | ESP32 `3V3` + all module `VIN/VCC` |
| **Black rail** | `GND` | ESP32 `GND` + all module `GND` + button leg 2 |

Use different holes of the same column for multiple wires. For example, column 1
has three connections: put the ESP32 wire in hole `1a`, the MAX30102 wire in
`1b` and the MLX90614 wire in `1c`. All three are already joined inside the
breadboard; no M-M jumper is needed for that column.

`GND*` in column 9 is the return from the 330 Ω resistor. Link column 9 to the
black `GND` rail with an **M-M** jumper.

## 2. ESP32 connections

All of these are **M-F jumpers**: female end on the ESP32 male header, male end
into the breadboard.

| ESP32 pin | Physical side | Connect to | Cable |
|---|---|---|---|
| `3V3` (pin 30) | right, bottom | red `+3V3` rail | M-F |
| `GND` (pin 29) | right, bottom | black `GND` rail | M-F |
| `D21` (pin 20) | right | breadboard **column 1** (`SDA`) | M-F |
| `D22` (pin 17) | right | breadboard **column 2** (`SCL`) | M-F |
| `D23` (pin 16) | right | breadboard **column 3** (`INT`) | M-F |
| `D34` (pin 4) | left | breadboard **column 4** (`HX_DT`) | M-F |
| `D25` (pin 8) | left | breadboard **column 5** (`HX_SCK`) | M-F |
| `D4` (pin 26) | right | breadboard **column 6** (`TARE`) | M-F |
| `D18` (pin 22) | right | breadboard **column 7** (`LED`) | M-F |
| `D35` (pin 5) | left | **not connected** in Format A | — |

## 3. MAX30102

| MAX30102 pin | Connect to | Cable |
|---|---|---|
| `VIN` | red `+3V3` rail | M-F |
| `GND` | black `GND` rail | M-F |
| `SDA` | breadboard **column 1** | M-F |
| `SCL` | breadboard **column 2** | M-F |
| `INT` | breadboard **column 3** | M-F |

## 4. MLX90614

| MLX90614 pin | Connect to | Cable |
|---|---|---|
| `VIN` | red `+3V3` rail | M-F |
| `GND` | black `GND` rail | M-F |
| `SDA` | breadboard **column 1** | M-F |
| `SCL` | breadboard **column 2** | M-F |

## 5. HX711

| HX711 pin | Connect to | Cable |
|---|---|---|
| `VCC` | red `+3V3` rail | M-F |
| `GND` | black `GND` rail | M-F |
| `DT` | breadboard **column 4** | M-F |
| `SCK` | breadboard **column 5** | M-F |

## 6. Button

Use a 4-leg tactile button. The two legs on the same side are internally joined;
use **diagonal legs** so the button is normally open.

| Button leg | Connect to | Cable |
|---|---|---|
| leg 1 (one side) | breadboard **column 6** | M-F |
| leg 2 (diagonal) | black `GND` rail | M-F |

If the button is plugged into the breadboard, put leg 1 in column 6 and leg 2 in
a free hole, then use an **M-M** jumper from that hole to the black rail.

## 7. LED + 330 Ω

The **long leg** of the LED is the anode (+); the **short leg** is the cathode
(−).

| LED / resistor | Connect to | Cable |
|---|---|---|
| LED anode (long) | breadboard **column 7** | M-F |
| LED cathode (short) | breadboard **column 8** | M-F or direct into the board |
| 330 Ω leg 1 | breadboard **column 8** | direct into the board |
| 330 Ω leg 2 | breadboard **column 9** | direct into the board |
| Column 9 | black `GND` rail | **M-M** |

If the LED and resistor are free, use M-F jumpers for the LED legs and short
M-M jumpers for the resistor legs. Do not connect the LED directly across 3V3
and GND without the 330 Ω resistor.

## 8. 50 kg load cell → HX711

No jumpers. Use the HX711 screw terminal and a small screwdriver.

| Load-cell wire | HX711 terminal |
|---|---|
| Red | `E+` |
| Black | `E−` |
| Green | `A+` |
| White | `A−` |

## 9. Cable types

| Type | Use |
|---|---|
| **M-F** (male–female) | ESP32 or free module male header → breadboard hole. This is the main cable. |
| **M-M** (male–male) | Only inside the breadboard: column 9 → GND rail, button leg → GND rail, or joining two columns/rails. |
| **F-F** (female–female) | **Not needed here.** It is only for joining two male headers directly without a breadboard. |
| **No jumper** | Load cell → HX711 screw terminal. |

## 10. Soldering

**No soldering iron needed** if:

- The ESP32 and all three modules already have their male headers soldered.
- You use the 400-point breadboard as the bus and M-F/M-M jumpers.
- The load cell goes to the HX711 screw terminal.

**You need a soldering iron** if:

- The modules came with loose headers that are not soldered.
- You use a perfboard instead of a solderless breadboard.
- You want a permanent connection or need to tin/shorten the load-cell wires.

## 11. Assembly order

1. Put the ESP32 and the three modules **beside** the breadboard.
2. Connect ESP32 `3V3` → red rail and `GND` → black rail (M-F).
3. Connect ESP32 `D21` → column 1 and `D22` → column 2 (M-F).
4. Connect MAX30102 `SDA` → column 1, `SCL` → column 2, `INT` → column 3 (M-F).
5. Connect MLX90614 `SDA` → column 1 and `SCL` → column 2 (M-F).
6. Connect every module `VIN/VCC` → red rail and `GND` → black rail (M-F).
7. Connect HX711 `DT` → column 4 and `SCK` → column 5 (M-F).
8. Connect the button to column 6 and GND; connect the LED to column 7 and the
   330 Ω resistor between columns 8 and 9, then column 9 to GND.
9. Connect the load cell to the HX711 screw terminal.
10. Before USB-C, check with a multimeter that `3V3` and `GND` are not shorted.
