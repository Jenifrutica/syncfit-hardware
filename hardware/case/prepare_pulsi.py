"""Prepare the Hunyuan mesh of Pulsi for the case.

The image-to-3D export has no texture, a flattened tip and a pointed bottom.
This script rotates it to Z-up, scales it to the case, rebuilds the tip,
paints it by projecting the reference views, flattens the bottom, decimates and
writes the printable parts.

    python3 hardware/case/prepare_pulsi.py ~/Downloads/pulsi.fbx

Needs `fbx2gltf` on PATH for .fbx input (or pass an already converted .glb).
Outputs in hardware/case/models/:
    pulsi_colored.glb, pulsi_case.stl, pulsi_stand.stl, pulsi_preview.png
"""

from __future__ import annotations

import argparse
import math
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np
import trimesh
from PIL import Image
from scipy import ndimage

HERE = Path(__file__).resolve().parent
OUT = HERE / "models"
VIEWS = HERE / "views"
HEIGHT = 135.0
TIP_FROM = 85.0          # desde aqui se reconstruye la punta
BOTTOM_CUT = 12.0        # corte plano de la base
FACES = 150_000
VOXEL = 0.8  # solo si se usa --voxel
BG = (255, 255, 255)


# ------------------------------------------------------------------ helpers
def load_mesh(src: Path) -> trimesh.Trimesh:
    if src.suffix.lower() == ".fbx":
        binary = shutil.which("fbx2gltf")
        if binary is None:
            sys.exit("fbx2gltf no esta en PATH: convierte el FBX a GLB primero")
        with tempfile.TemporaryDirectory() as tmp:
            glb = Path(tmp) / "mesh.glb"
            subprocess.run([binary, "-i", str(src), "-o", str(glb), "-b"],
                           check=True, capture_output=True)
            scene = trimesh.load(glb, force="scene")
    else:
        scene = trimesh.load(src, force="scene")
    mesh = scene.to_geometry() if hasattr(scene, "to_geometry") else scene.dump(concatenate=True)
    # el export trae un vertice por triangulo: sin soldarlos no se puede decimar
    mesh.merge_vertices()
    return mesh


def normalise(mesh: trimesh.Trimesh) -> trimesh.Trimesh:
    mesh = mesh.copy()
    mesh.apply_transform(trimesh.transformations.rotation_matrix(-np.pi / 2, [1, 0, 0]))
    mesh.apply_scale(HEIGHT / mesh.extents[2])
    mesh.apply_translation([-mesh.centroid[0], -mesh.centroid[1], -mesh.bounds[0][2]])
    return mesh


def rebuild_tip(mesh: trimesh.Trimesh) -> trimesh.Trimesh:
    """El modelo sale con una aleta plana arriba; la cierro a punta."""
    mesh = mesh.copy()
    v = mesh.vertices
    t = np.clip((HEIGHT - v[:, 2]) / (HEIGHT - TIP_FROM), 0.0, 1.0)
    factor = np.where(v[:, 2] > TIP_FROM, t ** 0.85, 1.0)
    v[:, 0] *= factor
    v[:, 1] *= factor
    mesh.vertices = v
    return mesh


def figure_box(path: Path) -> tuple[int, int, int, int]:
    rgb = np.asarray(Image.open(path).convert("RGB"))
    mask = ~background_mask(rgb)
    mask = ndimage.binary_closing(mask, iterations=6)
    mask = ndimage.binary_fill_holes(mask)
    ys, xs = np.nonzero(mask)
    return xs.min(), ys.min(), xs.max(), ys.max()


def background_mask(img: np.ndarray) -> np.ndarray:
    """Pixeles del fondo pastel (menta o rosa)."""
    rgb = img.astype(np.float32) / 255.0
    mx = rgb.max(2); mn = rgb.min(2)
    delta = np.maximum(mx - mn, 1e-6)
    sat = np.where(mx > 0, (mx - mn) / np.maximum(mx, 1e-6), 0.0)
    hue = np.where(mx == rgb[..., 0], ((rgb[..., 1] - rgb[..., 2]) / delta) % 6,
                   np.where(mx == rgb[..., 1],
                            (rgb[..., 2] - rgb[..., 0]) / delta + 2,
                            (rgb[..., 0] - rgb[..., 1]) / delta + 4)) / 6.0
    mint = (hue > 0.42) & (hue < 0.56)
    rose = (hue > 0.93) | (hue < 0.05)
    return (sat > 0.05) & (mint | rose)


def clean_background(img: np.ndarray) -> np.ndarray:
    """Rellena el fondo pastel con el color de figura mas cercano.

    Asi las alas y los bordes no toman el celeste o el rosa del fondo cuando se
    proyecta la foto sobre la malla.
    """
    rgb = img.astype(np.float32) / 255.0
    mx = rgb.max(2); mn = rgb.min(2)
    delta = np.maximum(mx - mn, 1e-6)
    sat = np.where(mx > 0, (mx - mn) / np.maximum(mx, 1e-6), 0.0)
    hue = np.where(mx == rgb[..., 0], ((rgb[..., 1] - rgb[..., 2]) / delta) % 6,
                   np.where(mx == rgb[..., 1],
                            (rgb[..., 2] - rgb[..., 0]) / delta + 2,
                            (rgb[..., 0] - rgb[..., 1]) / delta + 4)) / 6.0
    mint = (hue > 0.42) & (hue < 0.56)
    rose = (hue > 0.93) | (hue < 0.05)
    background = (sat > 0.05) & (mint | rose)
    if not background.any():
        return img
    index = ndimage.distance_transform_edt(background, return_distances=False,
                                           return_indices=True)
    out = img.copy()
    out[background] = img[index[0][background], index[1][background]]
    return out


def project_colors(mesh: trimesh.Trimesh, du: float = 0.0, dv: float = 0.0) -> np.ndarray:
    """Pinta cada vertice muestreando la vista frontal o la trasera."""
    lo, hi = mesh.bounds
    front = clean_background(np.asarray(Image.open(VIEWS / "ref_front.png").convert("RGB")))
    back = clean_background(np.asarray(Image.open(VIEWS / "ref_back.png").convert("RGB")))
    front = front.astype(float)
    back = back.astype(float)
    fb = figure_box(VIEWS / "ref_front.png")
    bb = figure_box(VIEWS / "ref_back.png")

    v = mesh.vertices
    u = np.clip((v[:, 0] - lo[0]) / max(hi[0] - lo[0], 1e-6) + du, 0, 1)
    w = np.clip((v[:, 2] - lo[2]) / max(hi[2] - lo[2], 1e-6) + dv, 0, 1)
    w = np.maximum(w, 0.09)   # la base no muestrea la sombra del pie

    def sample(img, box):
        x0, y0, x1, y1 = box
        # encoger un poco para no muestrear el fondo en los bordes
        dx, dy = (x1 - x0) * 0.02, (y1 - y0) * 0.02
        x0, x1 = x0 + dx, x1 - dx
        y0, y1 = y0 + dy, y1 - dy
        px = np.clip(x0 + u * (x1 - x0), 0, img.shape[1] - 1)
        py = np.clip(y0 + (1.0 - w) * (y1 - y0), 0, img.shape[0] - 1)
        return img[py.astype(int), px.astype(int)]

    # mezcla suave entre cara y espalda segun la normal, sin costura dura
    ny = mesh.vertex_normals[:, 1]
    weight = np.clip(0.5 - ny * 1.6, 0.0, 1.0)[:, None]
    colors = sample(front, fb) * weight + sample(back, bb) * (1.0 - weight)

    # las alas se fuerzan a blanco: la proyeccion siempre roza el fondo ahi
    maxx = np.abs(v[:, 0]).max()
    band = (v[:, 2] > 50) & (v[:, 2] < 100) & (np.abs(v[:, 1]) < 18)
    score = np.clip((np.abs(v[:, 0]) - 0.74 * maxx) / (0.14 * maxx), 0, 1) * band
    score = score[:, None]
    colors = colors * (1 - score) + np.array([250, 250, 252]) * score
    return np.clip(colors, 0, 255)


def flatten_bottom(mesh: trimesh.Trimesh, z: float) -> trimesh.Trimesh:
    return trimesh.intersections.slice_mesh_plane(
        mesh, plane_normal=[0, 0, 1], plane_origin=[0, 0, z], cap=True)


def stand(inner_radius: float) -> trimesh.Trimesh:
    base = trimesh.creation.cylinder(radius=50.0, height=16.0, sections=96)
    base.apply_translation([0, 0, 8.0])
    pocket = trimesh.creation.cylinder(radius=inner_radius + 0.4, height=14.0, sections=96)
    pocket.apply_translation([0, 0, 16.0 - 12.0 / 2 + 0.5])
    ring = trimesh.boolean.difference([base, pocket], engine="manifold")
    if isinstance(ring, list):
        ring = ring[0]
    return ring


def render(mesh: trimesh.Trimesh, colors: np.ndarray, path: Path,
           size: int = 768, forward=(0, 1, 0), up=(0, 0, 1), span: float = 180.0):
    f = np.asarray(forward, float); f /= np.linalg.norm(f)
    up = np.asarray(up, float)
    r = np.cross(f, up); r /= np.linalg.norm(r)
    v = np.cross(r, f)
    ss = 2
    w = size * ss
    scale = w / span
    centre = np.array([0.0, 0.0, HEIGHT / 2])

    img = np.ones((w, w, 3), np.float32)
    zbuf = np.full((w, w), np.inf, np.float32)
    light = -f * 0.75 + v * 0.48 - r * 0.28
    light /= np.linalg.norm(light)

    p = mesh.vertices - centre
    sx = ((p @ r) * scale + w / 2).astype(np.float32)
    sy = (-(p @ v) * scale + w / 2).astype(np.float32)
    d = p @ f

    tri = mesh.vertices[mesh.faces]
    normals = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
    ln = np.linalg.norm(normals, axis=1)
    good = ln > 1e-9
    normals = normals[good] / ln[good, None]
    flip = (normals @ (-f)) < 0
    normals[flip] *= -1
    lam = np.clip(normals @ light, 0, 1) * 0.62 + 0.38
    face_col = colors[mesh.faces[good]].mean(axis=1) * lam[:, None]

    for k, (a, b, c) in enumerate(mesh.faces[good]):
        x0, y0 = sx[a], sy[a]
        x1, y1 = sx[b], sy[b]
        x2, y2 = sx[c], sy[c]
        minx = max(int(math.floor(min(x0, x1, x2))), 0)
        maxx = min(int(math.ceil(max(x0, x1, x2))), w - 1)
        miny = max(int(math.floor(min(y0, y1, y2))), 0)
        maxy = min(int(math.ceil(max(y0, y1, y2))), w - 1)
        if minx > maxx or miny > maxy:
            continue
        area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0)
        if abs(area) < 1e-9:
            continue
        xs = np.arange(minx, maxx + 1) + 0.5
        ys = np.arange(miny, maxy + 1) + 0.5
        X, Y = np.meshgrid(xs, ys)
        w0 = ((x1 - X) * (y2 - Y) - (x2 - X) * (y1 - Y)) / area
        w1 = ((x2 - X) * (y0 - Y) - (x0 - X) * (y2 - Y)) / area
        w2 = 1.0 - w0 - w1
        mask = ((w0 >= 0) & (w1 >= 0) & (w2 >= 0)) | ((w0 <= 0) & (w1 <= 0) & (w2 <= 0))
        if not mask.any():
            continue
        z = w0 * d[a] + w1 * d[b] + w2 * d[c]
        sub = zbuf[miny:maxy + 1, minx:maxx + 1]
        better = mask & (z < sub)
        if not better.any():
            continue
        sub[better] = z[better]
        img[miny:maxy + 1, minx:maxx + 1][better] = face_col[k]

    Image.fromarray(np.clip(img, 0, 255).astype(np.uint8)).resize(
        (size, size), Image.LANCZOS).save(path)


def blockout() -> trimesh.Trimesh:
    """Silueta limpia: la gota del blockout mas las dos alas."""
    import importlib.util

    spec = importlib.util.spec_from_file_location("mk", HERE / "make_views.py")
    mk = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mk)

    parts = [mk.drop_mesh()]
    for sx in (-1, 1):
        parts.append(mk.superellipsoid((sx * (mk.radius_at(78) + 2), -3, 78),
                                       (13, 7, 17), 0.85, 0.85))
    verts, faces = [], []
    offset = 0
    for v, f in parts:
        verts.append(v)
        faces.append(f + offset)
        offset += len(v)
    mesh = trimesh.Trimesh(np.vstack(verts), np.vstack(faces), process=True)
    mesh.merge_vertices()
    return mesh


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("src", nargs="?", default=str(Path.home() / "Downloads" / "pulsi.fbx"))
    parser.add_argument("--blockout", action="store_true",
                        help="silueta limpia en vez de la malla de Hunyuan")
    args = parser.parse_args()
    src = Path(args.src).expanduser()
    prefix = "pulsi_clean" if args.blockout else "pulsi"
    OUT.mkdir(parents=True, exist_ok=True)

    if args.blockout:
        mesh = blockout()
    else:
        mesh = rebuild_tip(normalise(load_mesh(src)))

    # El export de Hunyuan trae un vertice por triangulo: hay que soldarlos
    # antes de decimar o el simplificador se come media malla.
    if args.blockout:
        # pocos vertices: subdividir antes de pintar para que el color sea suave
        low = mesh.subdivide()
        low = low.subdivide()
    else:
        low = mesh.simplify_quadric_decimation(face_count=FACES)
    low = low.process(validate=True)
    if not low.is_watertight:
        trimesh.repair.fill_holes(low)
        low.fix_normals()
    trimesh.smoothing.filter_taubin(low, lamb=0.5, nu=0.53, iterations=10)
    colors = project_colors(low)
    low.visual = trimesh.visual.ColorVisuals(
        mesh=low, vertex_colors=np.hstack([colors, np.full((len(colors), 1), 255)]))

    render(low, colors, OUT / f"{prefix}_preview.png")

    cut = 0.0 if args.blockout else BOTTOM_CUT
    body = flatten_bottom(low, cut) if cut > 0 else low
    body.visual = trimesh.visual.ColorVisuals()

    radius = 13.0
    for dz in (0.2, 0.6, 1.0):
        sec = body.section(plane_origin=[0, 0, cut + dz], plane_normal=[0, 0, 1])
        if sec is not None:
            pad, _ = sec.to_2D()
            radius = float(np.abs(pad.vertices).max())
            break

    body.export(OUT / f"{prefix}_case.stl")
    stand(radius).export(OUT / f"{prefix}_stand.stl")
    low.export(OUT / f"{prefix}_colored.glb")

    print(f"cuerpo: {len(body.faces)} caras, base Ø{2 * radius:.1f} mm, alto {body.extents[2]:.1f} mm")
    for suffix in ("case.stl", "stand.stl", "colored.glb", "preview.png"):
        print("wrote", OUT / f"{prefix}_{suffix}")


if __name__ == "__main__":
    main()
