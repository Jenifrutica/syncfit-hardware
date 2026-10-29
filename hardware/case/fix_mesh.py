"""Turn the Hunyuan mesh into a printable case.

Image-to-3D gives a nice organic body but no usable openings, so this script
scales it to the blockout, cuts every window and pocket with exact coordinates,
hollows it, opens the back for assembly and writes the STLs.

    pip install trimesh manifold3d numpy
    python3 hardware/case/fix_mesh.py hardware/case/hunyuan.glb

Outputs next to the input file:
    case_body.stl, case_back.stl, case_paddle.stl
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np

try:
    import trimesh
    from trimesh.boolean import difference, intersection, union
except ImportError:  # pragma: no cover
    sys.exit("falta trimesh: pip install trimesh manifold3d numpy")

# ---------------------------------------------------------------- parametros
TARGET = np.array([124.0, 84.0, 125.0])   # ancho x, fondo y, alto z (mm)
WALL = 2.0
BACK_Y = 30.0                             # donde empieza la abertura trasera

# Ventanas y bolsillos, en coordenadas del blockout.
# (nombre, centro, radio, eje)  eje None -> se ancla a la superficie frontal
CYLINDERS = [
    ("max_window", (0.0, -4.0, 124.0), 9.0, "z"),
    ("max_recess", (0.0, -4.0, 117.0), 12.0, "z"),
    ("mlx", (-18.0, None, 103.0), 6.0, "y"),
    ("led", (18.0, None, 103.0), 2.5, "y"),
]
BOXES = [
    ("usb", (0.0, 43.0, 26.0), (9.0, 16.0, 4.0)),
    ("cells", (0.0, 0.0, 7.0), (120.0, 46.0, 14.0)),
    ("paddle", (0.0, -42.0, 30.0), (62.0, 10.0, 40.0)),
]


def normalise(mesh: trimesh.Trimesh) -> trimesh.Trimesh:
    """Escala al tamano del blockout, centra en x/y y apoya en z = 0."""
    ext = mesh.bounds[1] - mesh.bounds[0]
    mesh = mesh.copy()
    mesh.apply_scale(float(np.min(TARGET / np.maximum(ext, 1e-6))))
    lo, hi = mesh.bounds
    mesh.apply_translation([-(lo[0] + hi[0]) / 2.0, -(lo[1] + hi[1]) / 2.0, -lo[2]])
    return mesh


def front_hit(mesh: trimesh.Trimesh, x: float, z: float) -> float:
    hits, _, _ = mesh.ray.intersects_location(
        np.array([[x, -500.0, z]]), np.array([[0.0, 1.0, 0.0]]))
    return float(hits[:, 1].min()) if len(hits) else -35.0


def make_cutters(mesh: trimesh.Trimesh):
    cutters = []
    for name, (cx, cy, cz), radius, axis in CYLINDERS:
        height = 44.0 if name == "max_window" else 22.0 if name == "max_recess" else 40.0
        cyl = trimesh.creation.cylinder(radius=radius, height=height, sections=64)
        if axis != "z":
            cyl.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
        if cy is None:
            cy = front_hit(mesh, cx, cz) - 2.0
        cyl.apply_translation([cx, cy, cz])
        cutters.append(cyl)
    for _, centre, size in BOXES:
        cutters.append(trimesh.creation.box(extents=size).apply_translation(centre))
    return cutters


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("mesh", type=Path)
    ap.add_argument("--wall", type=float, default=WALL)
    ap.add_argument("--solid", action="store_true", help="no ahuecar (prueba)")
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

    # abertura trasera para meter la electronica
    opening = trimesh.creation.box(extents=[96.0, 120.0, 76.0]).apply_translation(
        [0.0, 60.0 + BACK_Y, 52.0]
    )
    shell = difference(shell, opening)

    out = args.mesh.parent
    shell.export(out / "case_body.stl")
    print("wrote", out / "case_body.stl", f"({len(shell.faces)} caras)")

    # tapa trasera plana con labio y cuatro tornillos M3
    plate = trimesh.creation.box(extents=[104.0, 3.0, 84.0])
    lip = difference(
        trimesh.creation.box(extents=[97.0, 8.0, 77.0]),
        trimesh.creation.box(extents=[93.0, 10.0, 73.0]),
    )
    plate = union([plate, lip.apply_translation([0, BACK_Y - 5.0, 52.0])])
    holes = [
        trimesh.creation.cylinder(radius=1.7, height=20, sections=32)
        .apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
        .apply_translation([sx * 45.0, BACK_Y, 20.0 + sz * 32.0])
        for sx in (-1, 1)
        for sz in (0, 1)
    ]
    plate = difference(plate, holes).apply_translation([0.0, BACK_Y, 52.0])
    plate.export(out / "case_back.stl")
    print("wrote", out / "case_back.stl")

    paddle = trimesh.creation.icosphere(subdivisions=2, radius=1.0)
    paddle.apply_scale([31.0, 3.0, 20.0])
    paddle.apply_translation([0.0, -42.0, 30.0])
    paddle.export(out / "case_paddle.stl")
    print("wrote", out / "case_paddle.stl")


if __name__ == "__main__":
    main()
