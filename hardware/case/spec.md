# Case 3D — SyncFit Edge (mascota capibara)

Blockout y vistas para generar el cuerpo con Hunyuan3D y después cortar los
huecos reales sobre la malla.

- `make_views.py` — genera las vistas y el `blockout.stl`.
- `views/front|right|back|left|top|iso.png` — 768 × 768, fondo blanco, ortográficas.
- `blockout.stl` — malla de referencia (respaldo si Hunyuan no sirve).

## 1. Medidas del blockout

| Parte | Tamaño (mm) | Nota |
|---|---|---|
| Cuerpo | 124 × 84 × 76 | aloja protoboard 400 pts (84 × 55) |
| Cabeza | 80 × 70 × 66 | centro en z = 90 |
| Panza (agarre) | 66 × 14 × 42 | resalta 2 mm del cuerpo |
| Total | 124 × 84 × 125 | de z = 0 a z = 125 |

## 2. Mapa de huecos

Coordenadas sobre el blockout (x derecha, y hacia atrás, z arriba). El corte se
hace **después** de Hunyuan, con `fix_mesh.py`.

| Elemento | Centro (x, y, z) | Corte | Profundidad | Cara |
|---|---|---|---|---|
| MAX30102 (dedo) | (0, −4, 123) | Ø 18 mm | pasante al alojamiento | superior |
| MLX90614 (ojo izq.) | (−18, superficie, 103) | Ø 12 mm | 6 mm | frontal |
| LED (ojo der.) | (18, superficie, 103) | Ø 5 mm | 4 mm | frontal |
| USB-C | (0, 42, 26) | 9 × 4 mm | pasante | trasera |
| Celdas + barra | (0, 0, 8) | 120 × 46 × 12 | bolsillo | inferior |
| Paddle (pistón) | (0, −37, 30) | 62 × 40 | 2 mm de juego | frontal |

Detalles:

- **MAX30102**: ventana de 18 mm con un rebaje de 24 mm; el módulo queda 6 mm
  detrás. Sin filtro, o con acetato de 1 mm si se quiere cerrar.
- **MLX90614**: hueco abierto (el IR pasa por el aire). Si se tapa, usar
  polietileno de 0.05 mm, nunca acrílico.
- **USB-C**: alinear con el conector real; dejar 1 mm de holgura por lado.
- **Celdas**: el bolsillo lleva dos topes para que no giren y un canal para los
  cables hacia el HX711.

## 3. Hunyuan3D (image → 3D)

Subir las 4 vistas en este orden (es el que espera Hunyuan3D-2mv):

1. `views/front.png`
2. `views/back.png`
3. `views/left.png`
4. `views/right.png`

Ajustes sugeridos: 512 o 768 px, 50 steps, guidance 5.0, seed fija,
"remove background" activado.

Prompt (en inglés, funciona mejor):

```text
cute chibi capybara desktop device, chunky rounded body, small round ears,
short cream snout, stubby arms and feet, matte warm tan plastic,
soft studio lighting, plain white background, product design render,
single object, no text, no logo
```

Al terminar: descargar el `.glb` y guardarlo como `hardware/case/hunyuan.glb`.

## 4. Post-proceso

```bash
pip install trimesh manifold3d numpy
python3 hardware/case/fix_mesh.py hardware/case/hunyuan.glb
```

El script:

1. Escala la malla al tamaño del blockout y la centra.
2. Corta los huecos de la tabla con booleanos.
3. Ahueca la carcasa (pared 2 mm) y abre la tapa trasera.
4. Agrega los postes de tornillo M3 y el asiento del paddle.
5. Exporta `case_body.stl`, `case_lid.stl` y `case_paddle.stl`.

## 5. Impresión

| Ajuste | Valor |
|---|---|
| Material | PETG (mejor para el agarre) o PLA+ |
| Capa | 0.2 mm |
| Paredes | 3 perímetros (1.8 mm) |
| Relleno | 15 % |
| Soportes | solo en el hueco del dedo y el USB |
| Tolerancia | 0.25 mm en encajes, 0.35 mm en el paddle |

## 6. Ensamble

- Tapa trasera con 4 tornillos M3 × 10 y tuercas de inserción térmica.
- Paddle sobre dos pasadores de 3 mm; retorno con una lámina de PET de 0.5 mm.
- Las celdas van atornilladas al piso del bolsillo; el paddle apoya sobre su
  extremo libre.

> Las celdas deben quedar **en la misma dirección**. Si al apretar el valor baja,
> se invierte una celda o se intercambian `A+` y `A−` en el firmware.
