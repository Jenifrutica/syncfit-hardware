"""Carcasa de Pulsi en dos mitades atornilladas.

    python3 hardware/case/build_case.py

Salidas en models/:
    pulsi_front.stl   mitad delantera (ventanas, panza, bolsillos)
    pulsi_back.stl    mitad trasera (ranura USB, agujeros de paso)
    pulsi_stand.stl   soporte
"""

from __future__ import annotations

import importlib.util
from pathlib import Path

import numpy as np
import trimesh

HERE = Path(__file__).resolve().parent
MODELS = HERE / "models"
SRC = MODELS / "pulsi_case.stl"

WALL_SCALE = 0.88       # copia interior: 12 % hacia el centro
BELLY = (0.0, 95.0)     # ventana del dedo, ahora en la frente
EYE_L = (-20.7, 72.7)   # MLX90614
EYE_R = (17.3, 71.1)    # LED 5 mm
USB = (12.0, 6.0)       # ancho x, alto z
USB_Z = 52.0
POCKET = (36.0, 36.0)   # celda 34 x 34
POCKET_X = 24.0
POCKET_Z = 40.0
BOSSES = [(-44.0, 52.0), (44.0, 52.0), (-33.0, 92.0), (33.0, 92.0)]


def helpers():
    spec = importlib.util.spec_from_file_location("pp", HERE / "prepare_pulsi.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def tube_y(x: float, z: float, radius: float, y0: float, y1: float) -> trimesh.Trimesh:
    cyl = trimesh.creation.cylinder(radius=radius, height=abs(y1 - y0), sections=64)
    cyl.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
    cyl.apply_translation([x, (y0 + y1) / 2, z])
    return cyl


def half(mesh: trimesh.Trimesh, keep_positive: bool) -> trimesh.Trimesh:
    big = trimesh.creation.box(extents=[400, 400, 400])
    big.apply_translation([0, 200 if keep_positive else -200, 0])
    out = trimesh.boolean.intersection([mesh, big], engine="manifold")
    return out if isinstance(out, trimesh.Trimesh) else out[0]


def main() -> None:
    body = trimesh.load(SRC, force="mesh").process(validate=True)

    centre = body.bounds.mean(axis=0)
    inner = body.copy()
    inner.apply_transform(
        trimesh.transformations.translation_matrix(centre)
        @ np.diag([WALL_SCALE, WALL_SCALE, WALL_SCALE, 1.0])
        @ trimesh.transformations.translation_matrix(-centre))

    front = half(body, False)
    back = half(body, True)

    # ---- cavidad
    front = trimesh.boolean.difference([front, inner], engine="manifold")
    back = trimesh.boolean.difference([back, inner], engine="manifold")

    # ---- ventanas y bolsillos (mitad delantera)
    cuts = [
        tube_y(BELLY[0], BELLY[1], 6.0, -90, -20),
        tube_y(BELLY[0], BELLY[1], 10.0, -90, -38.5),
        tube_y(EYE_L[0], EYE_L[1], 6.0, -90, -20),
        tube_y(EYE_R[0], EYE_R[1], 3.0, -90, -20),
    ]

    # agujeros del pasador de la paleta, en los costados
    for sx in (-1, 1):
        pin = trimesh.creation.cylinder(radius=1.6, height=12, sections=24)
        pin.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [0, 1, 0]))
        pin.apply_translation([sx * 46, -18, 16])
        cuts.append(pin)
    # celda apoyada contra la pared interna, con placa de tope detras
    plates = []
    for sx in (-1, 1):
        hole = trimesh.creation.cylinder(radius=6.0, height=14, sections=32)
        hole.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
        hole.apply_translation([sx * 24, -40, 40])
        cuts.append(hole)
        plate = trimesh.creation.box(extents=[22, 3, 22])
        plate.apply_translation([sx * 24, -27, 40])
        plates.append(plate)
    front = trimesh.boolean.union([front] + plates, engine="manifold")
    front = trimesh.boolean.difference([front] + cuts, engine="manifold")

    # ---- ranura USB en la mitad trasera
    slot = trimesh.creation.box(extents=[USB[0], 60, USB[1]])
    slot.apply_translation([0, 30, USB_Z])
    back = trimesh.boolean.difference([back, slot], engine="manifold")

    # ---- postes con agujero piloto en el frente
    posts = []
    pilots = []
    for x, z in BOSSES:
        post = trimesh.creation.cylinder(radius=4.0, height=12, sections=32)
        post.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
        post.apply_translation([x, -6, z])
        posts.append(post)
        pilot = trimesh.creation.cylinder(radius=1.4, height=10, sections=24)
        pilot.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
        pilot.apply_translation([x, -5, z])
        pilots.append(pilot)
    front = trimesh.boolean.union([front] + posts, engine="manifold")
    front = trimesh.boolean.difference([front] + pilots, engine="manifold")

    clears = []
    for x, z in BOSSES:
        hole = trimesh.creation.cylinder(radius=1.7, height=40, sections=24)
        hole.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [1, 0, 0]))
        hole.apply_translation([x, 18, z])
        clears.append(hole)
    back = trimesh.boolean.difference([back] + clears, engine="manifold")

    # limpieza: quedarse con la pieza grande y quitar caras degeneradas
    def clean(part):
        pieces = part.split(only_watertight=False)
        part = max(pieces, key=lambda m: len(m.faces))
        part.update_faces(part.nondegenerate_faces())
        part.remove_unreferenced_vertices()
        return part

    front, back = clean(front), clean(back)

    for name, part in (("pulsi_front.stl", front), ("pulsi_back.stl", back)):
        part.export(MODELS / name)
        print(f"{name}: {len(part.faces)} caras, cerrado={part.is_watertight}, "
              f"volumen={part.volume/1000:.0f} cm3")

    pp = helpers()
    grey = np.tile([222.0, 216.0, 205.0], (len(front.vertices), 1))
    pp.render(front, grey, MODELS / "pulsi_front.png", span=200.0)
    pp.render(front, grey, MODELS / "pulsi_front_iso.png",
              forward=(-0.7, 1.0, -0.42), span=200.0)
    grey_b = np.tile([222.0, 216.0, 205.0], (len(back.vertices), 1))
    pp.render(back, grey_b, MODELS / "pulsi_back_iso.png",
              forward=(0.7, 1.0, -0.42), span=200.0)
    print("renders listos")


if __name__ == "__main__":
    main()
