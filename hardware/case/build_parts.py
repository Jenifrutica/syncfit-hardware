"""Tapa trasera de Pulsi y palancas del agarre.

La tapa sigue la curvatura del cuerpo (sale de la propia malla), lleva labio
para encajar en la abertura, ranura USB-C y cuatro tornillos M3.

    python3 hardware/case/build_parts.py
"""

from __future__ import annotations

import importlib.util
from pathlib import Path

import numpy as np
import trimesh

HERE = Path(__file__).resolve().parent
MODELS = HERE / "models"
SRC = MODELS / "pulsi_case.stl"

OPENING = (94, 62)       # ancho x, alto z de la abertura
OPENING_Z = 71.0         # centro en z
USB_Z = 40.0             # altura de la ranura en la tapa
USB = (12, 6)            # ancho x, alto z


def helpers():
    spec = importlib.util.spec_from_file_location("pp", HERE / "prepare_pulsi.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main() -> None:
    mesh = trimesh.load(SRC, force="mesh").process(validate=True)
    back = mesh.bounds[1][1]
    centre = mesh.bounds.mean(axis=0)

    # ---- tapa: la parte trasera del cuerpo, ahuecada 2.2 mm
    cap_box = trimesh.creation.box(extents=[104, 18, 72])
    cap_box.apply_translation([0, back - 7, 71])
    cap = trimesh.boolean.intersection([mesh, cap_box], engine="manifold")

    inner = mesh.copy()
    inner.apply_transform(
        trimesh.transformations.translation_matrix(centre)
        @ np.diag([0.955, 0.955, 0.955, 1.0])
        @ trimesh.transformations.translation_matrix(-centre))
    lid = trimesh.boolean.difference([cap, inner], engine="manifold")
    lid = lid.simplify_quadric_decimation(face_count=30000) if len(lid.faces) > 30000 else lid

    # ---- labio que entra en la abertura
    lip = trimesh.creation.box(extents=[OPENING[0], 8, OPENING[1]])
    hole = trimesh.creation.box(extents=[OPENING[0] - 4, 10, OPENING[1] - 4])
    ring = trimesh.boolean.difference([lip, hole], engine="manifold")
    ring.apply_translation([0, back - 12, OPENING_Z])
    lid = trimesh.boolean.union([lid, ring], engine="manifold")

    # ---- tornillos M3 y ranura USB-C
    cuts = []
    for sx in (-1, 1):
        for sz in (-26, 26):
            cyl = trimesh.creation.cylinder(radius=1.7, height=40, sections=32)
            cyl.apply_transform(
                trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
            cyl.apply_translation([sx * 40, back, OPENING_Z + sz])
            cuts.append(cyl)
    slot = trimesh.creation.box(extents=[USB[0], 40, USB[1]])
    slot.apply_translation([0, back, USB_Z])
    cuts.append(slot)
    lid = trimesh.boolean.difference([lid] + cuts, engine="manifold")

    lid.export(MODELS / "pulsi_lid.stl")
    print(f"tapa: {len(lid.faces)} caras, {lid.extents.round(1)}")

    pp = helpers()
    grey = np.tile([222.0, 216.0, 205.0], (len(lid.vertices), 1))
    pp.render(lid, grey, MODELS / "pulsi_lid.png", forward=(0, -1, 0),
              up=(0, 0, 1), span=200.0)
    print("render de la tapa listo")


if __name__ == "__main__":
    main()
