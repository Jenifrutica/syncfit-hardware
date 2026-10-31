# Impresión multicolor — Pulsi

STL no lleva color. Para imprimir en varios colores hay que dar **objetos
separados** y asignarle un filamento a cada uno.

## Archivos

| Archivo | Objetos | Volumen | Color |
|---|---|---|---|
| `pulsi_multicolor.stl` | 3 (cuerpo + 2 alas) | 120 cm³ | — |
| `body_amarillo.stl` | 1 | 84 cm³ | amarillo `#F5C95A` |
| `wings_blanco.stl` | 2 | 36 cm³ | blanco |

Tamaño del conjunto: **182 × 194 × 135 mm** · ≈ 152 g en PLA.

## Cómo asignar los colores

**PrusaSlicer / OrcaSlicer**
1. Importar `pulsi_multicolor.stl`.
2. Clic derecho → **Split to objects** (quedan 3 objetos).
3. Seleccionar las dos alas → asignar el filamento blanco.
4. Dejar el cuerpo en amarillo.

**Bambu Studio**
1. Importar y responder **Sí** a "cargar como varios objetos".
2. Seleccionar cada ala y ponerle el filamento 2 (blanco).

**Cura**
1. Importar; si no separa, usar la extensión *Mesh Tools*.
2. Con el plugin de multi-extrusión, asignar extrusor por objeto.

## Qué no se imprime y hay que pintar

| Detalle | Por qué |
|---|---|
| Rubor (mejillas) | Es un degradado suave, no un bloque de color |
| Boca y ojos | Trazos de 2–3 mm: se ven borrosos impresos |
| Aro de la panza | Es un relieve; si se quiere de otro color, hay que pintarlo |

Con acrílico o marcador permanente queda mejor que imprimirlo.

## Ensamble

- Las alas se pegan sobre las caras planas de los costados (x = ±55).
- El cuerpo es hueco (3–4 mm de pared) y **cerrado**: hay que abrirle la espalda
  antes de imprimir para meter la electrónica.
