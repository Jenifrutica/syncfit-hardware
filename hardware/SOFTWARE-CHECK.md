# Software ↔ hardware — qué se verificó y qué falta

Contraste entre lo **medido en el banco** (`hardware/TEST-LOG.md`, 2026-11-10) y
lo que espera el **firmware**, el **contrato** (`syncfit-contracts`) y la
**interfaz** (`/captura`).

## Mapa de los tres sensores

| Paso | Sensor | Bus | Dirección | Firmware | Interfaz |
|---|---|---|---|---|---|
| Pulso | MAX30102 (PPG) | I²C | `0x57` | `[ppg] finger=… hr=…` | dedo sobre el aro de la panza |
| Fuerza | HX711 + celda | GPIO | — | `[force] kg=…` | empujar Pulsi contra la base |
| Temperatura | MLX90614 (IR) | I²C | `0x5A` | `[ppg] temp=… dt=…` | frente sobre el ojo derecho |

## Lo que coincide

- **Direcciones I²C**: `SCAN` devolvió `0x57` y `0x5A`, igual que
  `include/board_pins.hpp` / los drivers.
- **Celda de carga**: `TARE` con offset `-242506` y `CAL 1` con
  `-31818 counts/kg`; vacío `-0.03 kg` y patrón de 1 kg `1.00 kg`. El firmware
  aplica `raw - offset` sobre `scale`, así que la interfaz puede confiar en kg
  reales.
- **Detección de dedo**: 12 de 12 muestras con `finger=1`; el paso de pulso de la
  interfaz pide cubrir el aro (misma condición).
- **Temperatura**: el MLX responde (20.7–21.5 °C en ambiente) y el firmware
  publica `temp` y `dt` (delta contra la primera lectura del arranque).
- **LED de estado**: destella cada 2 s, independiente del flujo de pantalla.

## Lo que no coincide todavía

| Tema | Medido | Qué espera el software | Acción |
|---|---|---|---|
| Pulso | 38–200 bpm, inestable | RMSSD estable para `syncfit-core` | Filtrar picos y descartar ventanas con dedo en movimiento (firmware) |
| Fuerza | 1 kg de patrón verificado | `isometric_force_loss_pct` solo si la base supera **5 kg** | La interfaz pide empujar firme; por debajo de 5 kg la pérdida queda en 0 |
| Temperatura | 21 °C en ambiente | `delta_temperature_c` en ±2 °C | El delta es contra el arranque: la frente debe apoyarse **después** de encender |
| Celdas sueltas | 3 kg dio 5.3 / 5.9 / 2.46 kg | Lectura repetible | Repetir `TARE` + `CAL` con las celdas ya montadas en el soporte |
| Telemetría | Sin Wi-Fi configurado | `ws://<host>:8000/api/v1/ws/telemetry` | `SET WIFI` y `SET WS <ip>,8000` antes de la demo |

## Qué hace hoy la interfaz

`/captura` (demostración pública) y `/app/checkin` (dentro de la app) recorren
los mismos tres pasos y **guardan el resultado en el dispositivo**
(`localStorage`, clave `syncfit-checkin`). Ese registro alimenta las tarjetas de
**Hoy** y **Rutina**, que muestran “verificado hoy” con los tres valores.

Los números del recorrido son **simulados** mientras el prototipo esté
desconectado: sirven para validar la interacción, no como medición. El camino
real ya existe y es el del firmware: cada 2.56 s envía un `TelemetryFrame 1.0.0`
por WebSocket con los biomarcadores derivados (`delta_temperature_c`,
`rmssd_hrv_ms`, `isometric_force_loss_pct`) y la ventana PPG de 256 muestras.

## Verificación en vivo (cuando se reconecte la placa)

```text
1. sudo chmod 666 /dev/ttyUSB0
2. pio device monitor -b 115200
3. SCAN                     → 0x57 y 0x5A
4. STATUS                   → offset y scale guardados
5. TARE (plataforma vacía)  → [force] tare ok
6. CAL 1 (peso patrón)      → [force] cal ok
7. dedo en el aro           → [ppg] finger=1 y hr estable
8. SET WIFI <ssid>,<pass>
9. SET WS <ip-del-backend>,8000
10. POST /api/v1/capture    → debe usar la última telemetría del dispositivo
```

Para el paso 10 falta un puente que hoy no existe: el WebSocket evalúa el frame
y responde la decisión, pero **no la guarda por usuaria**. Mientras eso no esté,
la rutina sigue usando el escenario simulado del backend.
