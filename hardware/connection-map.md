# SyncFit Edge — definitive connection map (Format A)

**Board:** ESP32 DevKit V1, generic 30-pin, USB-C.  
**Breadboard:** 400 points, used only as a bus; the ESP32 and modules stay free.  
**Power rails:** only one pair, labeled **X** and **Y**.  
**Cell join:** two 2-position screw blocks; green = `E+`/`E−`, black = `A+`/`A−`.  
**Diagram:** [`definitive.svg`](definitive.svg) · [`definitive.png`](definitive.png)

> **X = +3V3** · **Y = GND**  
> The red/black rails in the diagram are X and Y.

## 1. Breadboard columns

The five holes in one column (`a`–`e` above the center channel) are internally
connected. The rows are **not** connected across columns. Use only the top half
of the 400-point board.

| Column | Net | What connects there |
|---:|---|---|
| **1** | `SDA` | ESP32 `D21` + MAX30102 `SDA` + MLX90614 `SDA` |
| **2** | `SCL` | ESP32 `D22` + MAX30102 `SCL` + MLX90614 `SCL` |
| **3** | `D23` | ESP32 `D23` + MAX30102 `INT` |
| **4** | `D34` | ESP32 `D34` + HX711 `DT` |
| **5** | `D25` | ESP32 `D25` + HX711 `SCK` |
| **6** | spare | not used (no tare button) |
| **7** | `D18` | ESP32 `D18` + LED anode |
| **8** | `LED−` | LED cathode + 330 Ω leg 1 |
| **9** | `GND*` | 330 Ω leg 2 → **Y** with an M-M jumper |

Use different holes of the same column for multiple wires. Example: column 1
has `1a` = ESP32 `D21`, `1b` = MAX30102 `SDA`, `1c` = MLX90614 `SDA`.

## 2. ESP32 connections

All of these are **M-F jumpers**: female end on the ESP32 male header, male end
into the breadboard.

| ESP32 pin | Side | Connect to | Cable |
|---|---|---|---|
| `3V3` (pin 30) | right | rail **X** | M-F |
| `GND` (pin 29) | right | rail **Y** | M-F |
| `D21` (pin 20) | right | column **1** | M-F |
| `D22` (pin 17) | right | column **2** | M-F |
| `D23` (pin 16) | right | column **3** | M-F |
| `D34` (pin 4) | left | column **4** | M-F |
| `D25` (pin 8) | left | column **5** | M-F |
| `D18` (pin 22) | right | column **7** | M-F |
| `D35` (pin 5) | left | not connected | — |
| `D4` (pin 26) | right | not connected (no tare button) | — |

## 3. MAX30102

| Module pin | Connect to | Cable |
|---|---|---|
| `VIN` | rail **X** | M-F |
| `GND` | rail **Y** | M-F |
| `SDA` | column **1** | M-F |
| `SCL` | column **2** | M-F |
| `INT` | column **3** | M-F |

I2C address `0x57`, 100 Hz, 3.3 V only.

## 4. MLX90614

| Module pin | Connect to | Cable |
|---|---|---|
| `VIN` | rail **X** | M-F |
| `GND` | rail **Y** | M-F |
| `SDA` | column **1** | M-F |
| `SCL` | column **2** | M-F |

I2C address `0x5A`, 3.3 V variant.

## 5. HX711

| Module pin | Connect to | Cable |
|---|---|---|
| `VCC` | rail **X** | M-F |
| `GND` | rail **Y** | M-F |
| `DT` | column **4** | M-F |
| `SCK` | column **5** | M-F |

HX711 at **3.3 V**, 10 SPS, gain 128.

## 6. LED + 330 Ω

The **long leg** is the anode (+) and the **short leg** is the cathode (−). The
flat edge on the LED body also marks the cathode.

| LED / resistor | Connect to | Cable |
|---|---|---|
| LED anode (long) | column **7** | M-F |
| LED cathode (short) | column **8** | M-F or direct |
| 330 Ω leg 1 | column **8** (`8b`) | direct |
| 330 Ω leg 2 | column **9** (`9a`) | direct |
| Column **9** (`9b`) | rail **Y** | **M-M** |

`9a` and `9b` are two holes in the same column 9 and are internally connected.
`9a` holds the resistor leg; `9b` is only used for the M-M jumper to **Y**.

**If you do not have the resistor yet:** do not connect the bare LED. Use a
**220 Ω–1 kΩ** resistor (330 Ω recommended) or an LED module with a built-in
resistor. Without it, you can test the rest of the system; the LED is only the
status indicator.

## 7. Two 3-wire load cells → HX711

Use **two identical 3-wire cells**. Each cell is a **half-bridge**: the white
wire is the center; red and black are the ends.

```text
Cell A red   ─┐
Cell B red   ─┴── E+
Cell A black ─┐
Cell B black ─┴── E−
Cell A white ──── A+
Cell B white ──── A−
```

- Join the two red wires to `E+`.
- Join the two black wires to `E−`.
- One white wire to `A+`, the other to `A−`.
- If the reading goes negative when you press, swap `A+` and `A−`, or rotate one
  cell 180°.

**Before soldering**, verify the colors with a multimeter:

| Measurement | Expected for a half-bridge |
|---|---|
| red ↔ white | about R |
| black ↔ white | about R |
| red ↔ black | about 2R |

If those values do not match, the wire colors are not the assumed mapping. Do
not connect it until you confirm.

**Mechanics:** the two cells must be mounted so one flexes in the opposite
direction to the other. If both are loaded the same way, the signals cancel. The
final capacity depends on the mount: if the cells share the load, it can
approach 2× the single-cell rating; if one takes the full load, it remains the
single-cell rating.

### 7.1 Two 2-position terminal blocks

The build uses **two screw terminal blocks of 2 positions**, one **green** and one
**black**. Each block is two independent screws, so the four nets never touch:

| Block | Screw | Net | What goes in the screw |
|---|---|---|---|
| **1 · green** | 1 | `E+` | cell A red + cell B red + wire to HX711 `E+` |
| **1 · green** | 2 | `E−` | cell A black + cell B black + wire to HX711 `E−` |
| **2 · black** | 1 | `A+` | cell A white + wire to HX711 `A+` |
| **2 · black** | 2 | `A−` | cell B white + wire to HX711 `A−` |

```text
 red A ─┐
 red B ─┼─► [1·1 green] ──► HX711 E+
 wire  ─┘

 blk A ─┐
 blk B ─┼─► [1·2 green] ──► HX711 E−
 wire  ─┘

 wht A ─┐
 wire  ─┴─► [2·1 black] ──► HX711 A+
 wht B ─┐
 wire  ─┴─► [2·2 black] ──► HX711 A−
```

**Three wires do not fit in one screw.** Twist the two cell wires together with
the short wire that goes to the HX711 and tin the tip, so the bundle enters as a
single conductor. The `A+`/`A−` screws only take two wires and need no twisting
beyond keeping the pair short.

Rules:

- Colour-code the blocks: **green = excitation (`E`), black = signal (`A`)**.
- **Never** put an `E` net and an `A` net in the same block.
- If a block turns out to be a common busbar (every screw connected to every
  other), it cannot carry two different nets. Use it for a single net only.
- Two spare blocks are fine if you bought four; keep them as spares.

## 8. Tare and calibration without a button

There is **no tare button** in the definitive build. Leave GPIO4 unconnected;
the firmware enables the internal pull-up.

Open the serial monitor at **115200**:

```text
STATUS
TARE
CAL 5
```

- `TARE` with the cells empty.
- Put a **known weight** on the assembly and send `CAL 5` (or `CAL 10`).
- Wait for `calibration saved`.
- Do not calibrate with the maximum rated load; use a safe known weight.

## 9. Cable types

| Type | Use |
|---|---|
| **M-F** | ESP32 or free module male header → breadboard hole. Main cable. |
| **M-M** | Only inside the breadboard: `9b` → `Y`, or joining a column/rail. |
| **F-F** | Not needed. Only for two male headers directly without a breadboard. |
| **No jumper** | Load cells → 2-position blocks → HX711 (screwdriver only). |

## 10. Soldering

**No soldering iron needed** if:

- The ESP32 and sensor modules already have their male headers soldered.
- You use the 400-point board as a bus and M-F/M-M jumpers.
- The two load cells go to the two 2-position blocks and then to the HX711
  screw terminal.

**You need a soldering iron** if:

- The modules came with loose headers.
- You use a perfboard instead of a solderless breadboard.
- You want a permanent harness or need to tin the load-cell wires.

## 11. Bring-up order

1. Disconnect USB-C. Check continuity: `X`–`Y` must not be shorted.
2. Connect USB-C; measure about 3.3 V between X and Y.
3. I2C scan: expect `0x57` (MAX30102) and `0x5A` (MLX90614).
4. Wire both cells to the two 2-position blocks, then to the HX711; run `TARE`.
5. Add a known weight and run `CAL 5` or `CAL 10`.
6. Test the LED: anode on column 7, cathode on column 8, 330 Ω to column 9,
   column 9 to Y with M-M.
7. Send telemetry to the local WebSocket, not production, until validated.
