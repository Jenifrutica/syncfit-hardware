"""Colour the 50k Pulsi mesh and build a stand that fits it.

No deformation: the mesh is only rotated to Z-up, scaled to 135 mm, centred and
painted by projecting the reference views. The stand's seat follows the measured
profile of the bottom, so the body is not cut either.

    python3 hardware/case/finish_pulsi.py [~/Downloads/pulsii.glb] [--flip]
"""

from __future__ import annotations

import argparse
import importlib.util
import io
import json
import math
import shutil
import struct
import subprocess
from pathlib import Path

import numpy as np
import trimesh
from PIL import Image

HERE = Path(__file__).resolve().parent
OUT = HERE / "models"
HEIGHT = 135.0


def load_helpers():
    spec = importlib.util.spec_from_file_location("pp", HERE / "prepare_pulsi.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def glb_json(path: Path):
    """Devuelve (json, offset del chunk BIN) del GLB."""
    data = path.read_bytes()
    _, _, length = struct.unpack("<III", data[:12])
    off, js, bin_off = 12, None, None
    while off < length:
        clen, ctype = struct.unpack("<II", data[off:off + 8])
        body = off + 8
        if ctype == 0x4E4F534A:
            js = json.loads(data[body:body + clen])
        elif ctype == 0x004E4942:
            bin_off = body
        off = body + clen + ((4 - clen % 4) % 4 if clen % 4 else 0)
    return js, bin_off, data


def base_color_image(path: Path) -> Image.Image | None:
    """Extrae solo la textura base color declarada por el material."""
    js, bin_off, data = glb_json(path)
    material = (js.get("materials") or [{}])[0]
    index = (material.get("pbrMetallicRoughness", {})
             .get("baseColorTexture", {}).get("index"))
    if index is None:
        return None
    source = js["textures"][index]["source"]
    view = js["bufferViews"][js["images"][source]["bufferView"]]
    start = bin_off + view.get("byteOffset", 0)
    return Image.open(io.BytesIO(data[start:start + view["byteLength"]])).convert("RGB")


def texture_colors(src: Path, mesh: trimesh.Trimesh) -> np.ndarray | None:
    """Color por vertice muestreando la textura base color del GLB."""
    uv = getattr(mesh.visual, "uv", None)
    if uv is None:
        return None
    image = base_color_image(src)
    if image is None:
        return None
    atlas = np.asarray(image)
    h, w = atlas.shape[:2]
    u = np.clip(uv[:, 0] % 1.0, 0, 0.999)
    v = np.clip(uv[:, 1] % 1.0, 0, 0.999)
    px = (u * (w - 1)).astype(int)
    py = ((1.0 - v) * (h - 1)).astype(int)   # glTF guarda la V al reves
    return atlas[py, px].astype(float)


def as_glb(src: Path) -> Path:
    """trimesh no lee FBX: lo convierto con fbx2gltf si hace falta."""
    if src.suffix.lower() != ".fbx":
        return src
    binary = shutil.which("fbx2gltf") or str(Path.home() / ".local/bin/fbx2gltf")
    out = Path("/tmp/opencode") / (src.stem + ".glb")
    out.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run([binary, "-i", str(src), "-o", str(out), "-b"], check=True,
                   capture_output=True)
    return out


def prepare(src: Path, flip: bool) -> trimesh.Trimesh:
    scene = trimesh.load(as_glb(src), force="scene")
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

    src = Path(args.src).expanduser()
    mesh = prepare(src, args.flip)
    colors = texture_colors(as_glb(src), mesh)
    if colors is None:
        print("sin textura en el GLB: uso la proyeccion")
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
