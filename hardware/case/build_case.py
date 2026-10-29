"""Turn the solid Pulsi mesh into a printable case.

Hollows it, opens the back, cuts the sensor windows and the cell pockets, and
writes the parts. Nothing is deformed: every cut is a boolean on the original.

    python3 hardware/case/build_case.py
"""

from __future__ import annotations

import importlib.util
from pathlib import Path

import numpy as np
import trimesh

HERE = Path(__file__).resolve().parent
MODELS = HERE / "models"
SRC = MODELS / "pulsi_case.stl"

WALL = 2.0
# (nombre, centro (x, y, z), radio/medida)  -- medidos sobre el modelo
BELLY = (0.0, 44.0, 9.5)      # ventana del dedo (MAX30102): x, z, radio
EYE_L = (-20.7, 72.7, 6.0)     # ojo izquierdo (MLX90614)
EYE_R = (17.3, 71.1, 3.0)      # ojo derecho (LED 5 mm)
USB = (0.0, 34.0, 5.0)         # ranura USB-C: x, z, alto
CELL_Z = 42.0                  # altura del centro de las celdas
CELL_X = 24.0                  # separacion al centro


def load_helpers():
    spec = importlib.util.spec_from_file_location("pp", HERE / "prepare_pulsi.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def tube_y(x: float, z: float, radius: float, y0: float, y1: float) -> trimesh.Trimesh:
    """Cilindro a lo largo de Y, para perforar la cara frontal."""
    cyl = trimesh.creation.cylinder(radius=radius, height=abs(y1 - y0), sections=64)
    cyl.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
    cyl.apply_translation([x, (y0 + y1) / 2, z])
    return cyl


def main() -> None:
    mesh = trimesh.load(SRC, force="mesh")
    mesh = mesh.process(validate=True)
    print(f"cuerpo: {len(mesh.faces)} caras, {mesh.extents.round(1)}")

    # ---- cavidad interior: una gota escalada, sin tocar las alas
    lower = trimesh.creation.icosphere(subdivisions=4, radius=1.0)
    lower.apply_scale([46.0, 36.0, 28.0])
    lower.apply_translation([0.0, 0.0, 45.0])
    upper = trimesh.creation.icosphere(subdivisions=4, radius=1.0)
    upper.apply_scale([32.0, 28.0, 30.0])
    upper.apply_translation([0.0, 0.0, 80.0])
    cavity = trimesh.boolean.union([lower, upper], engine="manifold")
    shell = trimesh.boolean.difference([mesh, cavity], engine="manifold")

    # ---- abertura trasera para meter la electronica
    y_back = mesh.bounds[1][1]
    opening = trimesh.creation.box(extents=[94, 80, 62])
    opening.apply_translation([0, y_back - 8, 71])
    shell = trimesh.boolean.difference([shell, opening], engine="manifold")

    cuts = [
        tube_y(BELLY[0], BELLY[1], 6.0, -80, -18),          # ventana del dedo Ø12
        tube_y(BELLY[0], BELLY[1], 10.0, -80, -39.5),       # rebaje Ø20 para la yema
        tube_y(EYE_L[0], EYE_L[1], EYE_L[2], -80, -18),
        tube_y(EYE_R[0], EYE_R[1], EYE_R[2], -80, -18),
        trimesh.creation.box(extents=[10, 60, 5]).apply_translation([USB[0], 40, USB[1]]),
    ]
    # bolsillos de las celdas (34 x 34 x 7 + holgura), abiertos hacia atras
    for sx in (-1, 1):
        pocket = trimesh.creation.box(extents=[36, 12, 36])
        pocket.apply_translation([sx * CELL_X, 6, CELL_Z])
        cuts.append(pocket)

    shell = trimesh.boolean.difference([shell] + cuts, engine="manifold")
    if isinstance(shell, list):
        shell = max(shell, key=lambda m: len(m.faces))

    shell.export(MODELS / "pulsi_case_open.stl")
    print("cuerpo abierto:", len(shell.faces), "caras, watertight", shell.is_watertight)

    pp = load_helpers()
    grey = np.tile([222.0, 216.0, 205.0], (len(shell.vertices), 1))
    pp.render(shell, grey, MODELS / "pulsi_case_front.png", span=200.0)
    pp.render(shell, grey, MODELS / "pulsi_case_back.png", forward=(0, -1, 0), span=200.0)
    pp.render(shell, grey, MODELS / "pulsi_case_iso.png", forward=(-0.7, 1.0, -0.42), span=200.0)
    print("renders listos")


if __name__ == "__main__":
    main()
