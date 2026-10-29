"""Turn the Hunyuan mesh into a printable Pulsi case.

Image-to-3D gives a nice organic body but no usable openings, so this script
scales it to the blockout, cuts every window and pocket with exact coordinates,
hollows it, opens the back for assembly and writes the STLs.

    pip install trimesh manifold3d numpy
    python3 hardware/case/fix_mesh.py hardware/case/hunyuan.glb

Outputs next to the input file:
    case_body.stl, case_back.stl, case_wing_l/r.stl, case_stand.stl
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np

try:
    import trimesh
    from trimesh.boolean import difference, union
except ImportError:  # pragma: no cover
    sys.exit("falta trimesh: pip install trimesh manifold3d numpy")

# ---------------------------------------------------------------- parametros
TARGET = np.array([103.0, 87.5, 135.0])   # ancho x, fondo y, alto z (mm)
WALL = 2.0
BACK_Y = 26.0                             # inicio de la abertura trasera

# (nombre, centro, radio, eje)  eje "y" -> agujero frontal
CYLINDERS = [
    ("ppg",    (0.0, None, 45.0), 9.0, "y"),     # ventana del dedo (MAX30102)
    ("mlx",    (-16.0, None, 90.0), 5.5, "y"),   # ojo izquierdo (IR)
    ("led",    (16.0, None, 90.0), 2.5, "y"),    # ojo derecho
]
BOXES = [
    ("usb",   (0.0, 44.0, 34.0), (9.0, 16.0, 4.0)),      # ranura USB-C
    ("cells", (0.0, 0.0, 8.0), (92.0, 38.0, 16.0)),      # bolsillo de celdas
    ("hx",    (0.0, 8.0, 30.0), (36.0, 22.0, 12.0)),     # HX711 + borneras
]


def normalise(mesh: trimesh.Trimesh) -> trimesh.Trimesh:
    ext = mesh.bounds[1] - mesh.bounds[0]
    mesh = mesh.copy()
    mesh.apply_scale(float(np.min(TARGET / np.maximum(ext, 1e-6))))
    lo, hi = mesh.bounds
    mesh.apply_translation([-(lo[0] + hi[0]) / 2.0, -(lo[1] + hi[1]) / 2.0, -lo[2]])
    return mesh


def front_hit(mesh: trimesh.Trimesh, x: float, z: float) -> float:
    hits, _, _ = mesh.ray.intersects_location(
        np.array([[x, -500.0, z]]), np.array([[0.0, 1.0, 0.0]]))
    return float(hits[:, 1].min()) if len(hits) else -40.0


def make_cutters(mesh: trimesh.Trimesh):
    cutters = []
    for _, (cx, cy, cz), radius, axis in CYLINDERS:
        if cy is None:
            cy = front_hit(mesh, cx, cz) - 2.0
        cyl = trimesh.creation.cylinder(radius=radius, height=60.0, sections=64)
        if axis != "z":
            cyl.apply_transform(
                trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
        cyl.apply_translation([cx, cy + 24.0, cz])
        cutters.append(cyl)
    for _, centre, size in BOXES:
        cutters.append(trimesh.creation.box(extents=size).apply_translation(centre))
    return cutters


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("mesh", type=Path)
    ap.add_argument("--wall", type=float, default=WALL)
    ap.add_argument("--solid", action="store_true")
    args = ap.parse_args()

    body = trimesh.load(args.mesh, force="mesh")
    if isinstance(body, trimesh.Scene):
        body = body.dump(concatenate=True)
    body = body.process(validate=True)
    if not body.is_watertight:
        trimesh.repair.fill_holes(body)
        body.fix_normals()
    body = normalise(body)

    shell = difference(body, make_cutters(body))

    if not args.solid:
        centre = body.bounds.mean(axis=0)
        factor = 1.0 - (2.0 * args.wall / float(np.max(body.extents)))
        inner = body.copy()
        inner.apply_transform(
            trimesh.transformations.translation_matrix(centre)
            @ np.diag([factor, factor, factor, 1.0])
            @ trimesh.transformations.translation_matrix(-centre)
        )
        shell = difference(shell, inner)

    opening = trimesh.creation.box(extents=[80.0, 120.0, 90.0]).apply_translation(
        [0.0, 60.0 + BACK_Y, 55.0])
    shell = difference(shell, opening)

    out = args.mesh.parent
    shell.export(out / "case_body.stl")
    print("wrote", out / "case_body.stl", f"({len(shell.faces)} caras)")

    # tapa trasera con labio
    plate = trimesh.creation.box(extents=[82.0, 3.0, 92.0])
    lip = difference(
        trimesh.creation.box(extents=[79.0, 8.0, 89.0]),
        trimesh.creation.box(extents=[75.0, 10.0, 85.0]),
    ).apply_translation([0.0, BACK_Y - 5.0, 55.0])
    plate = union([plate, lip])
    holes = [
        trimesh.creation.cylinder(radius=1.7, height=20, sections=32)
        .apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
        .apply_translation([sx * 34.0, BACK_Y, 20.0 + sz * 62.0])
        for sx in (-1, 1)
        for sz in (0, 1)
    ]
    plate = difference(plate, holes).apply_translation([0.0, BACK_Y, 55.0])
    plate.export(out / "case_back.stl")
    print("wrote", out / "case_back.stl")

    # soporte: disco con asiento
    base = trimesh.creation.cylinder(radius=58.0, height=14.0, sections=96)
    base.apply_translation([0.0, 0.0, 7.0])
    pocket = trimesh.creation.cylinder(radius=34.0, height=12.0, sections=96)
    pocket.apply_translation([0.0, 0.0, 11.0])
    stand = difference(base, pocket)
    stand.export(out / "case_stand.stl")
    print("wrote", out / "case_stand.stl")

    # alas = paletas del agarre (piezas sueltas)
    for side, sx in (("l", -1), ("r", 1)):
        wing = trimesh.creation.icosphere(subdivisions=3, radius=1.0)
        wing.apply_scale([11.0, 6.0, 13.0])
        wing.apply_translation([sx * 52.0, -2.0, 72.0])
        wing.export(out / f"case_wing_{side}.stl")
        print("wrote", out / f"case_wing_{side}.stl")


if __name__ == "__main__":
    main()
