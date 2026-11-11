"""Render del frame inicial de Pulsi sobre fondo verde croma (para Gemini)."""

import math
import sys

import numpy as np
import trimesh
from PIL import Image

GREEN = (0, 177, 64)          # #00B140
SIZE = 1024
SS = 2
SRC = "hardware/case/models/pulsi_model.glb"
OUT = "hardware/case/render/pulsi_ref_green.png"


def render(mesh, colors, path, size=SIZE, forward=(0, 1, 0), up=(0, 0, 1), span=205.0):
    f = np.asarray(forward, float); f /= np.linalg.norm(f)
    up = np.asarray(up, float)
    r = np.cross(f, up); r /= np.linalg.norm(r)
    v = np.cross(r, f)
    w = size * SS
    scale = w / span
    centre = np.array([0.0, 0.0, 67.5])

    img = np.empty((w, w, 3), np.float32)
    img[:] = GREEN
    zbuf = np.full((w, w), np.inf, np.float32)
    light = -f * 0.72 + v * 0.52 - r * 0.26
    light /= np.linalg.norm(light)
    fill = -f * 0.55 - v * 0.2 + r * 0.5
    fill /= np.linalg.norm(fill)

    p = mesh.vertices - centre
    sx = ((p @ r) * scale + w / 2).astype(np.float32)
    sy = (-(p @ v) * scale + w / 2).astype(np.float32)
    d = p @ f

    tri = mesh.vertices[mesh.faces]
    n = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
    ln = np.linalg.norm(n, axis=1)
    good = ln > 1e-9
    n = n[good] / ln[good, None]
    flip = (n @ (-f)) < 0
    n[flip] *= -1
    lam = np.clip(n @ light, 0, 1) * 0.58 + np.clip(n @ fill, 0, 1) * 0.16 + 0.36
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

    Image.fromarray(np.clip(img, 0, 255).astype(np.uint8)).resize((size, size), Image.LANCZOS).save(path)
    return path


def main():
    s = trimesh.load(SRC, force="scene")
    mesh = s.to_geometry()
    assert hasattr(mesh.visual, "vertex_colors"), "sin colores"
    colors = np.asarray(mesh.visual.vertex_colors)[:, :3].astype(float)
    out = render(mesh, colors, OUT)
    print("escrito", out)


if __name__ == "__main__":
    sys.exit(main())
