"""Render the orthographic turnaround of the SyncFit mascot case.

The mesh here is a blockout built from superellipsoids: it fixes the
proportions, the sensor windows and the grip paddle so Hunyuan has a clean,
consistent set of views to work from. The real cut-outs are applied later to
the generated mesh with fix_mesh.py.

Run:  python3 hardware/case/make_views.py
Out:  hardware/case/views/{front,right,back,left,top,iso}.png + blockout.stl
"""

from __future__ import annotations

import math
from pathlib import Path

import numpy as np
from PIL import Image

SIZE = 768
SS = 2  # supersampling
OUT = Path(__file__).resolve().parent / "views"
OUT.mkdir(parents=True, exist_ok=True)

# ---------------------------------------------------------------- palette
BODY = (200, 166, 138)
BODY_DARK = (184, 146, 122)
CREAM = (239, 223, 210)
DARK = (46, 43, 56)
RECESS = (160, 122, 99)
PORT = (58, 58, 68)
ACCENT = (214, 120, 148)


# ---------------------------------------------------------------- geometry
def _cs(w, e):
    c = np.cos(w)
    s = np.sin(w)
    return np.sign(c) * np.abs(c) ** e, np.sign(s) * np.abs(s) ** e


def superellipsoid(center, radii, e1=0.45, e2=0.45, nu=64, nv=36):
    """Return (verts, faces) of a superellipsoid. e=1 sphere, e->0 box."""
    u = np.linspace(-np.pi, np.pi, nu, endpoint=False)
    v = np.linspace(-np.pi / 2, np.pi / 2, nv)
    cu, su = _cs(u, e2)
    cv, sv = _cs(v, e1)
    verts = np.empty((nv, nu, 3))
    verts[..., 0] = radii[0] * cv[:, None] * cu[None, :]
    verts[..., 1] = radii[1] * cv[:, None] * su[None, :]
    verts[..., 2] = radii[2] * sv[:, None] * np.ones_like(cu)[None, :]
    verts += np.asarray(center, dtype=float)
    verts = verts.reshape(-1, 3)

    faces = []
    for j in range(nv - 1):
        for i in range(nu):
            a = j * nu + i
            b = j * nu + (i + 1) % nu
            c = (j + 1) * nu + (i + 1) % nu
            d = (j + 1) * nu + i
            faces.append((a, b, c))
            faces.append((a, c, d))
    return verts, np.asarray(faces, dtype=np.int32)


def cylinder(center, radius, height, axis="y", nu=40, cap=True):
    """Capped cylinder along one axis, centred on `center`."""
    t = np.linspace(0, 2 * np.pi, nu, endpoint=False)
    ring = np.stack([np.cos(t) * radius, np.sin(t) * radius, np.zeros_like(t)], 1)
    if axis == "y":
        ring = ring[:, [0, 2, 1]]
        direction = np.array([0.0, 1.0, 0.0])
    elif axis == "x":
        ring = ring[:, [2, 1, 0]]
        direction = np.array([1.0, 0.0, 0.0])
    else:
        direction = np.array([0.0, 0.0, 1.0])
    lo = ring - direction * (height / 2)
    hi = ring + direction * (height / 2)
    verts = np.vstack([lo, hi]) + np.asarray(center, float)
    faces = []
    for i in range(nu):
        a, b = i, (i + 1) % nu
        c, d = nu + i, nu + (i + 1) % nu
        faces += [(a, b, d), (a, d, c)]
    if cap:
        lo_c = len(verts)
        hi_c = lo_c + 1
        verts = np.vstack([verts, np.asarray(center, float) - direction * (height / 2),
                           np.asarray(center, float) + direction * (height / 2)])
        for i in range(nu):
            faces.append((lo_c, (i + 1) % nu, i))
            faces.append((hi_c, nu + i, nu + (i + 1) % nu))
    return verts, np.asarray(faces, dtype=np.int32)


def front_y(verts, tx, tz, tol=2.5):
    """Y of the frontmost vertex of a mesh near (tx, tz)."""
    sel = (np.abs(verts[:, 0] - tx) < tol) & (np.abs(verts[:, 2] - tz) < tol)
    cand = verts[sel]
    if len(cand) == 0:
        return None
    return float(cand[cand[:, 1].argmin(), 1])


def model():
    parts = []  # (verts, faces, rgb)

    def add(geo, color):
        parts.append((geo[0], geo[1], np.asarray(color, float)))

    add(superellipsoid((0, 0, 38), (62, 42, 38), 0.42, 0.50), BODY)          # torso
    head = superellipsoid((0, -8, 90), (40, 35, 33), 0.62, 0.62)
    add(head, BODY)                                                          # head
    add(superellipsoid((0, -40, 82), (24, 13, 14), 0.55, 0.55), CREAM)       # snout
    add(cylinder((-7, -52, 88), 1.9, 6, axis="y", nu=16), DARK)              # nostril
    add(cylinder((7, -52, 88), 1.9, 6, axis="y", nu=16), DARK)               # nostril

    for sx in (-1, 1):                                                        # ears
        add(superellipsoid((sx * 26, -2, 116), (12, 8, 10), 0.8, 0.8), BODY_DARK)

    for sx in (-1, 1):                                                        # eyes
        y = front_y(head[0], sx * 18, 103)
        if y is not None:
            add(cylinder((sx * 18, y - 1.2, 103), 5.8, 4, axis="y", nu=28), DARK)
            if sx < 0:
                add(cylinder((sx * 18, y - 0.6, 103), 7.8, 2, axis="y", nu=28), ACCENT)
            else:
                add(cylinder((sx * 18, y - 1.4, 103), 2.6, 4, axis="y", nu=16), ACCENT)

    add(cylinder((0, -4, 121), 12.0, 5, axis="z", nu=36), RECESS)             # finger dimple
    add(cylinder((0, -4, 122.6), 9.0, 3, axis="z", nu=36), DARK)              # MAX window

    add(superellipsoid((0, -37, 30), (33, 7, 21), 0.35, 0.35), CREAM)         # grip paddle

    for sx in (-1, 1):                                                        # arms + feet
        add(superellipsoid((sx * 58, -16, 32), (10, 14, 18), 0.7, 0.7), BODY)
        add(superellipsoid((sx * 34, -32, 7), (17, 21, 8), 0.5, 0.5), BODY_DARK)

    add(superellipsoid((0, 41.5, 26), (7.0, 3.0, 4.0), 0.25, 0.25, 16, 10), PORT)  # USB-C

    return parts


# ---------------------------------------------------------------- renderer
def render(parts, forward, up, path, size=SIZE, ss=SS, center=(0, 0, 62),
           world_span=150.0):
    f = np.asarray(forward, float)
    f /= np.linalg.norm(f)
    up = np.asarray(up, float)
    r = np.cross(f, up)
    r /= np.linalg.norm(r)
    v = np.cross(r, f)

    w = size * ss
    scale = w / world_span
    c = np.asarray(center, float)

    img = np.ones((w, w, 3), np.float32)
    zbuf = np.full((w, w), np.inf, np.float32)

    # camera-relative key light so every view is evenly lit
    light1 = -f * 0.75 + v * 0.48 - r * 0.28
    light1 /= np.linalg.norm(light1)
    light2 = -f * 0.55 - v * 0.25 + r * 0.55
    light2 /= np.linalg.norm(light2)

    for verts, faces, color in parts:
        p = verts - c
        u = p @ r
        vv = p @ v
        d = p @ f
        sx = (u * scale + w / 2).astype(np.float32)
        sy = (-vv * scale + w / 2).astype(np.float32)

        tri = verts[faces]
        n = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
        ln = np.linalg.norm(n, axis=1)
        good = ln > 1e-9
        n = n[good] / ln[good, None]
        # two-sided lighting: flip normals that point away from the camera
        flip = (n @ (-f)) < 0
        n[flip] *= -1
        lam = np.clip(n @ light1, 0, 1) * 0.55 + np.clip(n @ light2, 0, 1) * 0.18 + 0.36
        base = np.asarray(color, float)[None, :]
        cols = np.clip(base * lam[:, None], 0, 255)

        faces_g = faces[good]
        for k, (a, b, cc) in enumerate(faces_g):
            x0, y0 = sx[a], sy[a]
            x1, y1 = sx[b], sy[b]
            x2, y2 = sx[cc], sy[cc]
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
            mask = (w0 >= 0) & (w1 >= 0) & (w2 >= 0)
            if not mask.any():
                continue
            z = w0 * d[a] + w1 * d[b] + w2 * d[cc]
            sub_z = zbuf[miny:maxy + 1, minx:maxx + 1]
            better = mask & (z < sub_z)
            if not better.any():
                continue
            sub_z[better] = z[better]
            img[miny:maxy + 1, minx:maxx + 1][better] = cols[k]

    out = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    out = out.resize((size, size), Image.LANCZOS)
    out.save(path)
    return path


def write_stl(parts, path):
    tris = []
    for verts, faces, _ in parts:
        tris.append(verts[faces])
    tris = np.vstack(tris)
    with open(path, "w", encoding="ascii") as fh:
        fh.write("solid syncfit_case\n")
        for t in tris:
            n = np.cross(t[1] - t[0], t[2] - t[0])
            ln = np.linalg.norm(n)
            n = n / ln if ln > 0 else np.zeros(3)
            fh.write(f"  facet normal {n[0]:.5f} {n[1]:.5f} {n[2]:.5f}\n    outer loop\n")
            for p in t:
                fh.write(f"      vertex {p[0]:.5f} {p[1]:.5f} {p[2]:.5f}\n")
            fh.write("    endloop\n  endfacet\n")
        fh.write("endsolid syncfit_case\n")


VIEWS = [
    ("front", (0, 1, 0), (0, 0, 1)),
    ("right", (-1, 0, 0), (0, 0, 1)),
    ("back", (0, -1, 0), (0, 0, 1)),
    ("left", (1, 0, 0), (0, 0, 1)),
    ("top", (0, 0, -1), (0, 1, 0)),
    ("iso", (0.7, 1.0, -0.45), (0, 0, 1)),
]

if __name__ == "__main__":
    parts = model()
    for name, fwd, up in VIEWS:
        p = render(parts, fwd, up, OUT / f"{name}.png")
        print("wrote", p)
    write_stl(parts, OUT.parent / "blockout.stl")
    print("wrote blockout.stl")
