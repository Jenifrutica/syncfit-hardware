# Registro de pruebas — SyncFit Edge (Formato A)

Pruebas hechas sobre el prototipo real con el firmware `0.1.0` y el montaje de
banco (celdas sueltas, todavía sin soporte mecánico).

## 2026-11-10 — Prueba completa de sensores

| Prueba | Comando / método | Resultado |
|---|---|---|
| Escaneo I2C | `SCAN` | **0x57** (MAX30102) + **0x5A** (MLX90614) ✅ |
| Temperatura IR | `[ppg] temp=` | 20.7 – 21.5 °C ✅ |
| Detección de dedo | `[ppg] finger=` | 12 de 12 muestras ✅ |
| Pulso | `[ppg] hr=` | 38 – 200 bpm, inestable ⚠️ |
| LED de estado | visual | destella cada 2 s ✅ |
| Celda: tare | `TARE` con plataforma vacía | offset = **-242506** ✅ |
| Celda: calibración | `CAL 1` con peso de 1 kg | scale = **-31818 counts/kg** ✅ |
| Celda: vacío | lectura sin peso | **-0.03 kg**, deriva 0.00 ✅ |
| Celda: peso patrón | lectura con 1 kg | **1.00 kg**, deriva 0.00 ✅ |

## Problemas encontrados y su causa

**1. El cero se contaminó al principio.** El primer `TARE` se hizo con el peso
de 3 kg encima, así que el vacío quedó en -1.18 kg. Se corrigió repitiendo el
`TARE` con la plataforma vacía y luego `CAL 1`. **El orden importa:**

```text
1. Plataforma vacía  →  TARE
2. Poner el peso     →  CAL <kg>
3. Quitar el peso    →  debe leer 0.00
```

**2. La celda no repetía con las celdas sueltas.** El mismo peso de 3 kg dio
5.3, 5.9 y 2.46 kg en intentos distintos. Es mecánico: si la carga se reparte
distinto, la lectura cambia. **Al montar las celdas en el soporte hay que
repetir `TARE` y `CAL`.**

**3. El pulso baila (38–200 bpm).** El dedo se detecta bien, pero el cálculo de
frecuencia salta. Causas: dedo en movimiento, luz ambiente, presión irregular.
No es hardware: es filtrado y detección de picos en el firmware.

## Cómo repetir la prueba

```text
SCAN              → deben salir 0x57 y 0x5A
STATUS            → muestra offset y scale guardados
TARE              → con la plataforma vacía
CAL 1             → con el peso patrón encima
```

El LED debe destellar cada 2 s. Sin Wi-Fi configurado no hay telemetría por
WebSocket; para eso hacen falta `SET WIFI` y `SET WS`.

## 2026-11-14 — Recalibración con el prototipo montado

| Prueba | Comando / método | Resultado |
|---|---|---|
| Escaneo I2C | `SCAN` | **0x57** + **0x5A** ✅ |
| TARE vacío | `TARE` con plataforma vacía | offset **-251970** ✅ |
| Peso del prototipo | prototipo sobre la plataforma | **0.18 kg** ✅ |
| TARE con prototipo | `TARE` con el prototipo montado | offset **-257412** ✅ |
| Escala | `CAL 1` con pesa de 1 kg sola | **-31137 counts/kg** ✅ |
| Pesa de 1 kg | lectura con 1 kg | **0.99–1.04 kg**, luego **1.00 kg** ✅ |
| Pesa de 3 lb | lectura con la pesa “3” | **1.29–1.34 kg** (teórico 1.36) ✅ |
| Empuje sobre el prototipo | presión suave con la mano | **3.3–5.8 kg** ✅ |
| Vacío después de cargar | retirar la pesa | **-0.20 kg** de histéresis ⚠️ |
| PPG con dedo | 21 muestras de 1 s | `finger=1` en **21/21**, HR **69–96**, media **78** ✅ |
| Temperatura frente | MLX a 2–5 cm | **34.0–34.4 °C**, `dt=+2.00` ✅ |
| Temperatura ambiente | alejar la frente | **19.6 °C**, `dt=-0.74` ✅ |
| Wi-Fi + WebSocket | `SET WIFI` + `SET WS` + `REBOOT` | conecta a **192.168.80.51**, `/live` y `/capture` con `capture_source=device` ✅ |

### Cambios de firmware de esta sesión

- Segundo `DcBlocker` a **0.95** (~0.8 Hz) para quitar la oscilación lenta de la presión del dedo.
- Media móvil de **8 muestras** antes del detector de picos.
- Detección de dedo con **300 ms de histéresis**: sin dedo, `hr=0` y el detector se reinicia.
- Timestamp por muestra dentro del FIFO (10 ms), no por tick de tarea.
- HR por **mediana** de los últimos intervalos, no por el último.
- Refractario del detector: **450 ms** para no contar el pico dicrótico del mismo latido.
- Comando serie **`PPG`** para volcar la ventana de 256 muestras.
- Stack de la tarea de red: **4 KB → 8 KB** (el frame de telemetría desbordaba la pila y reiniciaba la placa).

### Notas mecánicas

- El prototipo pesa **0.18 kg** y su carga sí llega a las celdas.
- Después de retirar la pesa de 1 kg, el cero se movió **-0.20 kg**. Es histéresis del montaje:
  **hacer `TARE` con la plataforma vacía (o con el prototipo puesto) antes de cada captura.**
- La escala es lineal y quedó guardada; al montar el prototipo **no hace falta recalibrar**, solo `TARE`.
- El buffer del backend reporta `dropped` creciente porque guarda solo los últimos 10 s
  y el dispositivo envía ventanas de 2.56 s; no son frames perdidos.

## 2026-11-14 — Repetición final paso a paso

Secuencia completa repetida con el prototipo ya montado. Estos son los valores
que quedan guardados en NVS:

| Paso | Resultado |
|---|---|
| I²C | `0x57` + `0x5A` ✅ |
| TARE vacío | offset **-262005**, **0.00 kg** ✅ |
| Pesa de 1 kg sola | **0.91 kg** antes de calibrar |
| `CAL 1` | escala **-28305 counts/kg**; con 1 kg → **1.00 kg** ✅ |
| Vacío tras retirar la pesa | **-0.10 kg** (histéresis); `TARE` → **0.00 kg** |
| Peso del prototipo | **0.08 kg** |
| TARE con prototipo | offset **-261131**, **0.00 kg** ✅ |
| Empuje sobre el prototipo | pico **10.06 kg**, línea base **8.66 kg**, pérdida hasta **41.5 %** ✅ |
| PPG | `finger=1` en **16/16**, HR **70–90 bpm**, media **83** ✅ |
| Temperatura frente | **32–47 °C** (según distancia), `dt=+2.00` ✅ |
| Temperatura ambiente | **20.2–20.7 °C**, `dt=-1.34..-0.84` ✅ |
| WebSocket | `/live` con timestamp y 256 muestras; `/capture` → `capture_source=device` ✅ |

Notas de esta repetición:

- El umbral de arranque de la serie bajó a **1.5 kg** porque el empuje cómodo
  sobre el prototipo ronda 1.7–10 kg; con 5 kg no siempre arrancaba.
- La pérdida isométrica se calcula sola: primer empuje ≥1.5 kg → ventana de
  2.5 s para la línea base → `%` de pérdida; al soltar 2 s vuelve a cero.
- El comando `START` reinicia la serie manualmente.

