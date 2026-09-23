# Hardware design

Physical design artifacts for the two form factors.

## Format A — fixed check-in station (totem / grip station)

- Mains/USB powered, on a warm-up desk.
- MAX30102 (finger PPG) and MLX90614 (thermal) plus a grip handle on the HX711
  load cell.
- Maximum optical stability; easiest to demonstrate to an evaluation panel.

## Format B — wearable wrist band

- Elastic ergonomic band keeping the optical and thermal sensors on the wrist
  during dynamic warm-up, with a coupling for the grip dynamometer.
- LiPo battery; continuous monitoring.

## To be added

- [ ] Wiring diagram
- [ ] Pinout
- [ ] Schematic
- [ ] Bill of materials (BOM)

## Placeholder pinout (ESP32 DevKit)

| Sensor | Signal | ESP32 pin |
|--------|--------|-----------|
| MAX30102 | SDA / SCL | 21 / 22 |
| MLX90614 | SDA / SCL | 21 / 22 (shared I2C) |
| HX711 | DT / SCK | 34 / 25 |
