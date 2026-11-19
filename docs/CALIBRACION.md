# Sesión de calibración del prototipo

Guía paso a paso para validar y calibrar el prototipo contra producción
(`wss://api.syncfit.jenilarper.dev/api/v1/ws/telemetry`). Todo se puede hacer
sin el monitor interactivo, con dos herramientas de este repo:

- `tools/serial_cmd.py`: manda comandos por USB y muestra la respuesta.
- `tools/live_watch.py`: lee `GET /live` de producción (fuerza rápida, pulso,
  dedo y temperatura) y al final da mín, máx, media y desviación de la fuerza.

## 0. Conexión

```bash
ls /dev/ttyUSB* /dev/ttyACM*            # debe aparecer el CH340 (ttyUSB0)
python3 tools/serial_cmd.py STATUS SCAN  # SCAN debe listar 0x57 (MAX30102) y 0x5A (MLX90614)
```

Si cambiaste de red Wi-Fi (el firmware guarda `YCM202302`):

```bash
python3 tools/serial_cmd.py --wait 2 "SET WIFI <red>,<clave>" REBOOT
python3 tools/serial_cmd.py --boot 8 STATUS   # wifi=1 ws=1
```

## 1. Firmware con canal rápido de fuerza

```bash
pio test -e native                 # 20/20
pio run -e esp32dev -t upload      # flashea d782a66 o posterior
python3 tools/serial_cmd.py --boot 8 STATUS
python3 tools/live_watch.py --seconds 5   # "edad" < 0.3 s = llega el mensaje force cada 100 ms
```

## 2. Cero (TARE) y deriva

Plataforma en reposo, sin tocar:

```bash
python3 tools/serial_cmd.py --wait 6 TARE
python3 tools/live_watch.py --seconds 30   # desviación < 0.05 kg, media ≈ 0
```

Carga y descarga varias veces y repite la lectura en reposo: si la media se aleja
más de **0.5 kg** del cero, revisa que la plataforma no toque la base y que las
celdas no estén torsionadas (HANDOFF §9.5). El botón **Poner a cero** de la app
hace lo mismo que `TARE`.

## 3. Escala (CAL)

Solo si con un peso conocido la lectura se desvía más de ~3 %:

```bash
python3 tools/serial_cmd.py --wait 6 TARE        # sin peso
# coloca la pesa (p. ej. 1 kg) sola sobre la plataforma
python3 tools/serial_cmd.py --wait 8 "CAL 1"
python3 tools/live_watch.py --seconds 10         # media ≈ 1.00 kg
```

Anota el `offset` y la `escala` que imprime `STATUS` en HANDOFF §4.2.

## 4. Latencia de fuerza

Con `/app/checkin` abierto en el paso de fuerza, empuja y suelta: el número debe
seguir al empuje en menos de ~0.3 s. En paralelo,
`python3 tools/live_watch.py --hz 10` muestra la edad de cada lectura.

## 5. Pulso

Yema quieta en el MAX30102, reloj en la otra muñeca, 60 s:

```bash
python3 tools/live_watch.py --seconds 60   # columna "pulso" vs. reloj
```

Diferencia aceptable: ±5 bpm. Si se desvía, ajustar `estimateHeartRateBpm()` en
`include/filters.hpp` (rango 50–120, mediana 3 s) y repetir con `PPG` para volcar
la ventana.

## 6. Temperatura

Sensor a 2–5 cm del ojo izquierdo: la temperatura del frame debe quedar estable
(±0.3 °C) durante 30 s.
