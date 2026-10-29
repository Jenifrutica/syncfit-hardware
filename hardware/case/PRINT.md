# Piezas para imprimir — Pulsi

Carcasa en **dos mitades atornilladas** (se abre para meter la electrónica) más
el soporte. Nada está deformado: todo sale de la malla de Hunyuan con cortes
booleanos.

## Archivos y material

| Archivo | Volumen | Tamaño | Tiempo aprox.* |
|---|---|---|---|
| `pulsi_front.stl` | 82 cm³ | 162 × 47 × 135 mm | 7–8 h |
| `pulsi_back.stl` | 89 cm³ | 162 × 44 × 134 mm | 7–9 h |
| `pulsi_stand.stl` | 114 cm³ | 100 × 100 × 22 mm | 5–6 h |
| **Total** | **285 cm³** | | **19–24 h** |

\* Con 0.2 mm de capa, 15 % de relleno y 3 paredes. Para cotizar, el dato que
sirve es el **peso de filamento**: ≈ 360 g en PETG (1.27 g/cm³ con relleno).

## Ajustes de impresión

| Ajuste | Valor |
|---|---|
| Material | **PETG** (o PLA+ si no hay) |
| Capa | 0.2 mm |
| Paredes | 3 perímetros |
| Relleno | 15 % |
| Soportes | sí, en la mitad trasera (es un cuenco) |
| Brim | sí, 5 mm (las dos mitades son altas y estrechas) |

### Orientación

| Pieza | Cómo ponerla |
|---|---|
| `pulsi_front` | **boca abajo** (la cara del plano de corte sobre la cama) — las ventanas quedan arriba |
| `pulsi_back` | **boca abajo** también, el cuenco hacia arriba (necesita soportes dentro) |
| `pulsi_stand` | plano, tal cual |

## Tornillería

- 4 tornillos **M3 × 12** (autoperforantes o con tuerca).
- Los agujeros piloto del frente son Ø 2.8 mm; los de paso de la espalda, Ø 3.4 mm.
- Posiciones: (±44, z 52) y (±33, z 92).

## Qué entra y dónde

| Componente | Zona |
|---|---|
| MAX30102 (20 × 15) | panza, detrás de la ventana Ø12 |
| MLX90614 (16 × 12, lente Ø10) | ojo izquierdo, detrás del Ø12 |
| LED 5 mm | ojo derecho, Ø6 |
| ESP32 + protoboard | cavidad central |
| HX711 | cavidad, junto al ESP32 |
| 2 celdas 34 × 34 × 7 | contra la pared interna, detrás de la panza |
| USB-C | ranura en la mitad trasera (z = 52) |

## Pendiente antes de imprimir

1. **Alas móviles**: hoy las alas van macizas con el cuerpo. Para que aprieten
   las celdas hay que decidir el mecanismo:
   - **bisagra impresa** (ala aparte + pasador Ø3), o
   - **ala flexible** impresa en TPU.
2. **Pasacables** de las celdas al HX711.
3. Probar el encaje de las dos mitades (0.3 mm de holgura en el plano).
