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
