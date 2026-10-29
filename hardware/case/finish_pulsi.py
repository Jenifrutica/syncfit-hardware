"""Colour the 50k Pulsi mesh and build a stand that fits it.

No deformation: the mesh is only rotated to Z-up, scaled to 135 mm, centred and
painted by projecting the reference views. The stand's seat follows the measured
profile of the bottom, so the body is not cut either.

    python3 hardware/case/finish_pulsi.py [~/Downloads/pulsii.glb] [--flip]
"""

from __future__ import annotations

import argparse
import importlib.util
import math
from pathlib import Path

import numpy as np
import trimesh

HERE = Path(__file__).resolve().parent
OUT = HERE / "models"
HEIGHT = 135.0


def load_helpers():
    spec = importlib.util.spec_from_file_location("pp", HERE / "prepare_pulsi.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def prepare(src: Path, flip: bool) -> trimesh.Trimesh:
    scene = trimesh.load(src, force="scene")
    mesh = scene.to_geometry()
    mesh.apply_transform(trimesh.transformations.rotation_matrix(math.pi / 2, [1, 0, 0]))
    if flip:
        mesh.apply_transform(trimesh.transformations.rotation_matrix(math.pi, [0, 0, 1]))
    mesh.apply_scale(HEIGHT / mesh.extents[2])
    mesh.apply_translation([-mesh.centroid[0], -mesh.centroid[1], -mesh.bounds[0][2]])
    return mesh


def bottom_profile(mesh: trimesh.Trimesh, top: float = 24.0, step: float = 1.5):
    """Radio del cuerpo en funcion de z, para el asiento del soporte."""
    profile = []
    z = 0.4
    while z <= top:
        sec = mesh.section(plane_origin=[0, 0, z], plane_normal=[0, 0, 1])
        if sec is not None:
            pad, _ = sec.to_2D()
            profile.append((float(np.abs(pad.vertices).max()), z))
        z += step
    return profile


def lathe(profile, segments: int = 96) -> trimesh.Trimesh:
    verts, faces = [], []
    for r, z in profile:
        for j in range(segments):
            a = 2 * np.pi * j / segments
            verts.append([r * math.cos(a), r * math.sin(a), z])
    verts = np.asarray(verts, float)
    n = len(profile)
    for i in range(n - 1):
        for j in range(segments):
            a = i * segments + j
            b = i * segments + (j + 1) % segments
            c = (i + 1) * segments + (j + 1) % segments
            d = (i + 1) * segments + j
            faces += [(a, b, c), (a, c, d)]
    # tapas superior e inferior
    bottom = len(verts); verts = np.vstack([verts, [0, 0, profile[0][1]]])
    top = len(verts); verts = np.vstack([verts, [0, 0, profile[-1][1]]])
    for j in range(segments):
        faces.append((bottom, (j + 1) % segments, j))
        base = (n - 1) * segments
        faces.append((top, base + j, base + (j + 1) % segments))
    return trimesh.Trimesh(verts, np.asarray(faces), process=True)


def stand(profile, clearance: float = 0.7, height: float = 22.0) -> trimesh.Trimesh:
    base = trimesh.creation.cylinder(radius=50.0, height=height, sections=96)
    base.apply_translation([0, 0, height / 2])
    pocket = trimesh.creation.revolve(
        [(0.0, profile[0][1])] + [(r + clearance, z) for r, z in profile] + [(0.0, height)],
        sections=96)
    return trimesh.boolean.difference([base, pocket], engine="manifold")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("src", nargs="?", default=str(Path.home() / "Downloads" / "pulsii.glb"))
    ap.add_argument("--flip", action="store_true", help="girar 180 grados si la cara queda atras")
    ap.add_argument("--du", type=float, default=0.0, help="ajuste fino horizontal de la proyeccion")
    ap.add_argument("--dv", type=float, default=0.0, help="ajuste fino vertical de la proyeccion")
    args = ap.parse_args()

    pp = load_helpers()
    OUT.mkdir(parents=True, exist_ok=True)

    mesh = prepare(Path(args.src).expanduser(), args.flip)
    colors = pp.project_colors(mesh, du=args.du, dv=args.dv)
    rgba = np.hstack([colors, np.full((len(colors), 1), 255)]).astype(np.uint8)
    mesh.visual = trimesh.visual.ColorVisuals(mesh=mesh, vertex_colors=rgba)
    mesh.export(OUT / "pulsi_model.glb")
    pp.render(mesh, colors, OUT / "pulsi_preview.png")
    pp.render(mesh, colors, OUT / "pulsi_iso.png", forward=(-0.65, 1.0, -0.4))

    body = mesh.copy()
    body.visual = trimesh.visual.ColorVisuals()
    body.export(OUT / "pulsi_case.stl")

    prof = bottom_profile(mesh)
    seat = stand(prof)
    seat.export(OUT / "pulsi_stand.stl")

    print(f"cuerpo: {len(body.faces)} caras, {body.extents[0]:.0f} x {body.extents[1]:.0f} x {body.extents[2]:.0f} mm")
    print(f"soporte: asiento hasta r={prof[-1][0]:.1f} en z={prof[-1][1]:.0f}")
    for name in ("pulsi_model.glb", "pulsi_case.stl", "pulsi_stand.stl",
                 "pulsi_preview.png", "pulsi_iso.png"):
        print("wrote", OUT / name)


if __name__ == "__main__":
    main()
