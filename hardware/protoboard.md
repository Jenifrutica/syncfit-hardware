# SyncFit Edge — protoboard assembly (Format A)

**Board:** ESP32 DevKit V1, generic 30-pin, USB-C.  
**Assumption:** solderless breadboard (830 points) and modules with **pre-soldered male headers**.  
**Diagram:** [`protoboard.svg`](protoboard.svg) · [`protoboard.png`](protoboard.png)

> If your modules have loose headers or you use a perfboard instead of a
> solderless breadboard, you need a soldering iron. See "Do I need a soldering
> iron?" below.

## 1. Jumper types

| Type | What it is | Where to use it |
|---|---|---|
| **M-M** | Male–male | Breadboard row to breadboard row, power rails, short bus links |
| **M-F** | Male–female | ESP32 male pin → breadboard hole; module male pin → breadboard if the module is not plugged in |
| **F-F** | Female–female | Only module-to-module when both sides have male pins; not needed in this breadboard layout |

In this build:

- The **ESP32 stays beside the breadboard**. Use **M-F** jumpers: female end on
  the ESP32 male pin, male end into the breadboard.
- The **MAX30102, MLX90614 and HX711 plug directly into the breadboard** using
  their male headers. No jumper is needed at the module pin.
- Use **M-M** jumpers to link breadboard rows, the I2C bus and the power rails.
- The **50 kg load cell** goes to the HX711 screw terminal. No jumper and no
  soldering iron are required; use a small screwdriver.

## 2. Breadboard layout

```
Top power rail:  +3V3 (red)  ·  GND (black)
Top block:       MAX30102 (cols 1-5)   MLX90614 (cols 7-10)
                 button (cols 12-13)   LED + 330Ω (cols 15-19)
Center channel
Bottom block:    HX711 (cols 1-4)
Bottom power:    +3V3 / GND, linked to the top rails with M-M
```

Place the modules so their pins sit in different columns. Each column's five
holes (a–e above, f–j below) are internally connected.

## 3. Connection list

### ESP32 → breadboard (all M-F)

| Net | ESP32 pin | Breadboard destination | Jumper |
|---|---|---|---|
| `3V3` | `3V3` (right, pin 30) | top `+3V3` rail | M-F |
| `GND` | `GND` (right, pin 29) | top `GND` rail | M-F |
| `SDA` | `GPIO21` (right, pin 20) | SDA bus column (MAX/MLX SDA) | M-F |
| `SCL` | `GPIO22` (right, pin 17) | SCL bus column (MAX/MLX SCL) | M-F |
| `MAX_INT` | `GPIO23` (right, pin 16) | MAX30102 `INT` column | M-F |
| `HX_DT` | `GPIO34` (left, pin 4) | HX711 `DT` column (bottom block) | M-F |
| `HX_SCK` | `GPIO25` (left, pin 8) | HX711 `SCK` column (bottom block) | M-F |
| `TARE` | `GPIO4` (right, pin 26) | button column | M-F |
| `LED` | `GPIO18` (right, pin 22) | LED anode column | M-F |
| `BAT_ADC` | `GPIO35` (left, pin 5) | not connected in Format A | — |

### On the breadboard (all M-M)

| Connection | From | To | Jumper |
|---|---|---|---|
| MAX30102 power | MAX `VIN` | top `+3V3` rail | M-M |
| MAX30102 ground | MAX `GND` | top `GND` rail | M-M |
| MLX90614 power | MLX `VIN` | top `+3V3` rail | M-M |
| MLX90614 ground | MLX `GND` | top `GND` rail | M-M |
| I2C SDA bus | MAX `SDA` | MLX `SDA` + ESP32 `SDA` | M-M + M-F |
| I2C SCL bus | MAX `SCL` | MLX `SCL` + ESP32 `SCL` | M-M + M-F |
| HX711 power | HX711 `VCC` | bottom `+3V3` rail | M-M |
| HX711 ground | HX711 `GND` | bottom `GND` rail | M-M |
| Button ground | button leg 2 | bottom `GND` rail | M-M |
| LED cathode | LED cathode | 330 Ω → bottom `GND` rail | M-M |
| Pull-up SDA | 4.7 kΩ | `SDA` bus ↔ `+3V3` | M-M |
| Pull-up SCL | 4.7 kΩ | `SCL` bus ↔ `+3V3` | M-M |

**I2C pull-ups:** add the two 4.7 kΩ resistors only if the MAX30102/MLX90614
modules do not already include them. Do not duplicate pull-ups.

**Load cell:** HX711 screw terminal `E+ / E- / A+ / A-` to the load-cell wires
`red / black / green / white`. Use a screwdriver, not a soldering iron.

## 4. Do I need a soldering iron?

### No, for a pure breadboard build if:

- The ESP32 already has its male headers soldered (your photo shows the headers
  in place).
- The MAX30102, MLX90614 and HX711 modules already have their male headers
  soldered.
- You use a breadboard and M-M/M-F jumpers.
- The load cell goes to the HX711 screw terminal.

### Yes, if:

- The modules came with **loose headers** that must be soldered before use.
- You use a **perfboard / protoboard with soldered connections** instead of a
  solderless breadboard.
- You want a permanent build, a PCB, or want to shorten/tin the load-cell wires.
- You need to solder a connector to the load cell because your HX711 has a
  4-pin header instead of a screw terminal.

**Tools for the no-solder build:** breadboard, M-M and M-F jumpers, small
screwdriver for the HX711 terminal, USB-C cable, multimeter (recommended).
**Optional:** 4.7 kΩ and 330 Ω resistors, button, LED.

## 5. Bring-up order

1. With USB-C disconnected, check for shorts between `3V3` and `GND`.
2. Connect USB-C and measure ~3.3 V between the `3V3` and `GND` rails.
3. Run an I2C scan: expect `0x57` (MAX30102) and `0x5A` (MLX90614).
4. Connect the HX711 and load cell; run `TARE`.
5. Put a known weight on the load cell and run `CAL <kg>`.
6. Test the button and LED.
7. Send telemetry to the local WebSocket, not production, until the hardware
   is validated.

**Safety:** HX711 at 3.3 V only. Do not feed `VIN` and USB-C at the same time.
The 50 kg load cell is the rated limit; use a mechanical stop and avoid maximum
grip tests without it.
