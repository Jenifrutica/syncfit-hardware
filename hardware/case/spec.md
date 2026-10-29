# Case 3D — SyncFit Edge (Pulsi)

Dos objetivos distintos:

| Uso | Qué se usa |
|---|---|
| Página web / render bonito | `views/ref_*.png` o el 3D que generes con Hunyuan |
| **Case para imprimir** | `blockout.stl` + `fix_mesh.py` con las medidas reales |

## 1. Medidas reales de la electrónica

| Pieza | Medida (mm) | Dónde va |
|---|---|---|
| ESP32 DevKit 30 pines | 55 × 28 × 13 (+ headers 20) | centro, USB-C al fondo |
| Protoboard 400 pts | 84 × 55 × 9 | apoyada en el piso interno |
| HX711 | 34 × 21 × 12 | al lado del ESP32 |
| MAX30102 | 15 × 20 × 5 | detrás de la panza |
| MLX90614 (GY-906) | 21 × 15 × 6 | detrás del ojo izquierdo |
| LED 3 mm | 3 × 5 | ojo derecho |
| 2 borneras 2P | 10 × 10 × 10 c/u | junto al HX711 |
| 2 celdas 3 hilos | 75 × 20 × 6 c/u | piso, una por ala |
| Barra del agarre | 60 × 15 × 4 | sobre las celdas |
| Peso total aprox. | 180 g | |

Cuerpo: **103 × 87.5 × 135 mm**, base plana **Ø 60**.
Soporte: **Ø 116 × 14 mm**, asiento hundido Ø 34 a 9 mm de altura.

## 2. Mapa de huecos

Coordenadas: x derecha, **y hacia atrás** (la cara mira a −y), z arriba desde la base.

| Elemento | Centro | Corte | Cara |
|---|---|---|---|
| MAX30102 (dedo) | (0, −43.7, 45) | Ø 18 + aro Ø 22 | panza |
| MLX90614 (temperatura) | (−16, −31.0, 90) | Ø 11 | ojo izquierdo |
| LED | (16, −31.0, 90) | Ø 5 | ojo derecho |
| USB-C | (0, +43.6, 34) | 9 × 4 | espalda |
| Celdas | (0, 0, 8) | bolsillo 92 × 38 × 16 | piso interno |
| HX711 + borneras | (0, 8, 30) | 36 × 22 × 12 | piso interno |
| Tapa trasera | y > 26 | 82 × 92 | espalda |
| Asiento del soporte | z = 0 | Ø 60 plano | base |

## 3. Flujo de toma de datos

El case está diseñado para que cada paso tenga un lugar físico:

| Paso | Gesto | Sensor | Zona de Pulsi |
|---|---|---|---|
| 1 | Apoyar la yema del dedo | MAX30102 (PPG) | **panza**, el aro |
| 2 | Apretar las alas | 2 celdas + HX711 | **alas**, empujan las celdas |
| 3 | Acercar la carita a la sien/frente | MLX90614 (IR) | **ojo izquierdo** |
| Estado | Mira el LED | LED de estado | **ojo derecho** |

Si prefieres que la temperatura se tome por detrás, el hueco se mueve a la
espalda (z ≈ 95) y el ojo izquierdo queda ciego. Es un cambio de 5 minutos.

## 4. Mecanismo del agarre

```text
ala izquierda ──pasador Ø3──► brazo interior ──► celda A (extremo libre)
ala derecha   ──pasador Ø3──► brazo interior ──► celda B (extremo libre)
```

- Celdas atornilladas al piso, **en la misma dirección**.
- Carrera 3–4 mm; retorno por la elasticidad de la celda + lámina PET 0.5 mm.
- Si al apretar el valor baja, se invierte una celda o se cambian `A+` / `A−`.

## 5. Modelo de Hunyuan (`prepare_pulsi.py`)

```bash
# requiere fbx2gltf en PATH para .fbx
python3 hardware/case/prepare_pulsi.py ~/Downloads/pulsi.fbx
```

Salidas en `models/`:

| Archivo | Qué es |
|---|---|
| `pulsi_colored.glb` | modelo con color (vértices) para la página |
| `pulsi_case.stl` | cuerpo imprimible, base plana Ø 26 |
| `pulsi_stand.stl` | soporte Ø 100 con asiento Ø 27 |
| `pulsi_preview.png` | vista previa del resultado |

El script: suelda los vértices (el FBX trae uno por triángulo), escala a 135 mm,
reconstruye la punta, decima a 150 k caras, suaviza, **pinta proyectando
`ref_front.png` y `ref_back.png`** y aplana la base en z = 12.

> El FBX de Hunyuan es un relieve 2.5D: la cara y la espalda se unen en el
> ecuador, así que queda una costura visible y el perfil es más romboidal que la
> referencia. Para el case impreso se puede usar el blockout, que tiene la
> silueta correcta.

## 6. Soporte

Disco Ø 116 × 14 con asiento hundido Ø 34 a 9 mm. El cuerpo apoya su base plana
de Ø 60 dentro del asiento y queda de pie.

- Se imprime aparte, sin soportes.
- Opcional: fieltro o goma en la cara inferior para que no resbale.

## 7. Hunyuan3D (image → 3D)

Juego de referencia: `views/ref_front.png`, `ref_back.png`, `ref_profile.png`,
`ref_threequarter.png`, `ref_top.png` (salen de `~/Downloads/pulsito.jpeg` con
`split_refs.py`).

Recomendado: **single-view con `ref_front.png`**. Si usas multivista:
front, back, profile, threequarter — en ese orden.

```text
cute mascot character, yellow teardrop body with a pointed top, two small white
wings on the sides, two round navy eyes, rosy blush cheeks, small smile,
smooth matte plastic, soft studio lighting, plain white background,
product design render, single object, no text
```

## 8. Post-proceso e impresión

```bash
pip install trimesh manifold3d numpy
python3 hardware/case/fix_mesh.py hardware/case/hunyuan.glb
```

Salidas: `case_body.stl`, `case_back.stl`, `case_wing_l/r.stl`, `case_stand.stl`.

| Ajuste | Valor |
|---|---|
| Material | PETG |
| Capa | 0.2 mm |
| Paredes | 3 perímetros |
| Relleno | 15 % |
| Soportes | solo en la punta y la ranura USB |
| Tolerancia | 0.25 mm encajes, 0.35 mm alas |
