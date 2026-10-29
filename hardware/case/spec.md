# Case 3D — SyncFit Edge (Pulsi)

Pulsi es la mascota del sitio: una gota amarilla con ojos azul marino, mejillas
rosadas, sonrisa, dos alitas blancas y pies pequeños. Este directorio convierte
esa ilustración en una carcasa imprimible.

- `make_views.py` — genera las vistas y el `blockout.stl`.
- `views/front|right|back|left|top|iso.png` — 768 × 768, fondo blanco, ortográficas.
- `views/turnaround.png` — las cuatro vistas juntas.
- `blockout.stl` — malla de referencia (respaldo si Hunyuan falla).

## 1. Medidas del blockout

| Parte | Tamaño (mm) | Nota |
|---|---|---|
| Cuerpo | 103 × 87.5 × 135 | gota; aloja la protoboard 400 pts (84 × 55) |
| Alas | 22 × 12 × 26 c/u | paletas del agarre, piezas sueltas |
| Pies | 13 × 17 × 11 c/u | base, no portantes |
| Pared | 2 mm | PETG |

Coordenadas: x derecha, **y hacia atrás** (la cara frontal mira a −y), z arriba.

## 2. Mapa de huecos

| Elemento | Centro (x, y, z) | Corte | Cara |
|---|---|---|---|
| MAX30102 (dedo) | (0, −43.7, 45) | Ø 18 pasante | frontal (panza) |
| MLX90614 (ojo izq.) | (−16, −31.0, 90) | Ø 11 pasante | frontal |
| LED (ojo der.) | (16, −31.0, 90) | Ø 5 | frontal |
| USB-C | (0, +43.6, 34) | 9 × 4 | trasera |
| Celdas + barra | (0, 0, 8) | 92 × 38 × 16 | bolsillo inferior |
| HX711 + borneras | (0, 8, 30) | 36 × 22 × 12 | interior |
| Tapa trasera | y > 26 | 82 × 92 | trasera |

Detalles:

- **Ojos**: cada ojo es una ventana. El izquierdo lleva el MLX90614 (IR pasa por
  el aire, sin filtro); el derecho lleva el LED de estado, que brilla a través.
- **Panza**: el dedo se apoya en el aro; el módulo queda 5 mm detrás.
- **USB-C**: alinear con el conector real; 1 mm de holgura por lado.
- **Alas**: no se pegan; van sobre un pasador Ø3 en el cuerpo y presionan una
  celda cada una.

## 3. Mecanismo del agarre

```text
   ala izquierda ──pasador Ø3──► brazo interior ──► celda A (extremo libre)
   ala derecha   ──pasador Ø3──► brazo interior ──► celda B (extremo libre)
```

- Las celdas van atornilladas al piso del bolsillo, en la **misma dirección**.
- Apretar las alas deforma las dos celdas; el retorno lo da su propia
  elasticidad más una lámina de PET de 0.5 mm.
- Carrera útil: 3–4 mm. Si el valor no sube al apretar, se invierte una celda o
  se intercambian `A+` y `A−`.

## 4. Hunyuan3D (image → 3D)

Hay dos juegos de imágenes:

| Juego | Archivos | Para qué |
|---|---|---|
| **Referencia** (render bonito) | `views/ref_front.png`, `ref_threequarter.png`, `ref_profile.png`, `ref_back.png`, `ref_top.png` | alimentar Hunyuan |
| **Blockout** (geometría) | `views/front.png`, `back.png`, `left.png`, `right.png`, `top.png` | medir y cortar |

El juego de referencia sale de `~/Downloads/pulsito.jpeg` con `split_refs.py`.

Para Hunyuan, la mejor combinación es **single-view con `ref_front.png`** (es la
toma más limpia). Si usas el modelo multivista, sube en este orden:

1. `views/ref_front.png`
2. `views/ref_back.png`
3. `views/ref_profile.png`
4. `views/ref_threequarter.png`

Ajustes sugeridos: 768 px, 50 steps, guidance 5.0, seed fija, quitar fondo.

Prompt:

```text
cute mascot character, yellow teardrop body with a pointed top, two small white
wings on the sides, two round navy eyes, rosy blush cheeks, small smile,
two tiny navy feet, smooth matte plastic, soft studio lighting, plain white
background, product design render, single object, no text
```

Guardar el resultado como `hardware/case/hunyuan.glb`.

## 5. Post-proceso

```bash
pip install trimesh manifold3d numpy
python3 hardware/case/fix_mesh.py hardware/case/hunyuan.glb
```

Salidas: `case_body.stl`, `case_back.stl`, `case_wing_l.stl`, `case_wing_r.stl`.

El script escala al blockout, corta los huecos de la tabla con booleanos, ahueca
la carcasa, abre la tapa trasera y separa las alas.

## 6. Impresión

| Ajuste | Valor |
|---|---|
| Material | PETG (mejor para el agarre) o PLA+ |
| Capa | 0.2 mm |
| Paredes | 3 perímetros |
| Relleno | 15 % |
| Soportes | solo en la punta y la ranura USB |
| Tolerancia | 0.25 mm en encajes, 0.35 mm en las alas |

## 7. Ensamble

- Tapa trasera con 4 tornillos M3 × 10.
- Alas sobre pasadores Ø3, con tope para que no giren.
- Celdas atornilladas al piso; el brazo del ala apoya en su extremo libre.
- Pies pegados o a presión (no soportan carga).
