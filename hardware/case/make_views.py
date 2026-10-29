"""Render the orthographic turnaround of the SyncFit case: Pulsi.

Pulsi is the site mascot: a yellow teardrop with navy eyes, blush cheeks, a
smile, two little white wings and stubby feet. This blockout turns that
illustration into a printable shell:

  * teardrop body, 104 wide x 88 deep x 135 tall
  * eyes = MLX90614 window (left) and status LED (right)
  * belly = MAX30102 finger window
  * wings = the two grip paddles that press the load cells
  * back = USB-C slot

Run:  python3 hardware/case/make_views.py
Out:  hardware/case/views/*.png + blockout.stl
"""

from __future__ import annotations

import math
from pathlib import Path

import numpy as np
from PIL import Image

SIZE = 768
SS = 2
OUT = Path(__file__).resolve().parent / "views"
OUT.mkdir(parents=True, exist_ok=True)

# ---------------------------------------------------------------- palette
YELLOW = (245, 201, 90)
YELLOW_DARK = (232, 184, 73)
NAVY = (43, 42, 74)
BLUSH = (240, 160, 142)
WHITE = (250, 250, 252)
RECESS = (214, 172, 66)
PORT = (52, 52, 70)

H = 135.0          # alto total
R = 56.0           # radio maximo (la Bezier alcanza ~0.92 R)
SQUASH = 0.85      # profundidad = 0.85 x ancho


# ---------------------------------------------------------------- geometry
def _cs(w, e):
    c, s = np.cos(w), np.sin(w)
    return np.sign(c) * np.abs(c) ** e, np.sign(s) * np.abs(s) ** e


def superellipsoid(center, radii, e1=0.45, e2=0.45, nu=48, nv=28):
    u = np.linspace(-np.pi, np.pi, nu, endpoint=False)
    v = np.linspace(-np.pi / 2, np.pi / 2, nv)
    cu, su = _cs(u, e2)
    cv, sv = _cs(v, e1)
    verts = np.empty((nv, nu, 3))
    verts[..., 0] = radii[0] * cv[:, None] * cu[None, :]
    verts[..., 1] = radii[1] * cv[:, None] * su[None, :]
    verts[..., 2] = radii[2] * sv[:, None]
    verts += np.asarray(center, float)
    verts = verts.reshape(-1, 3)
    faces = []
    for j in range(nv - 1):
        for i in range(nu):
            a = j * nu + i
            b = j * nu + (i + 1) % nu
            c = (j + 1) * nu + (i + 1) % nu
            d = (j + 1) * nu + i
            faces += [(a, b, c), (a, c, d)]
    return verts, np.asarray(faces, np.int32)


def cylinder(center, radius, height, axis="y", nu=40, cap=True):
    t = np.linspace(0, 2 * np.pi, nu, endpoint=False)
    ring = np.stack([np.cos(t) * radius, np.sin(t) * radius, np.zeros_like(t)], 1)
    if axis == "y":
        ring, direction = ring[:, [0, 2, 1]], np.array([0.0, 1.0, 0.0])
    elif axis == "x":
        ring, direction = ring[:, [2, 1, 0]], np.array([1.0, 0.0, 0.0])
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
        verts = np.vstack([verts,
                           np.asarray(center, float) - direction * (height / 2),
                           np.asarray(center, float) + direction * (height / 2)])
        for i in range(nu):
            faces.append((lo_c, (i + 1) % nu, i))
            faces.append((hi_c, nu + i, nu + (i + 1) % nu))
    return verts, np.asarray(faces, np.int32)


# La silueta es una Bezier cubica en (r, z): gota con base redonda y punta fina.
_P0 = (30.0, 0.0)     # base plana de 60 mm para apoyar en el soporte
_P1 = (56.0, 22.0)
_P2 = (50.0, 80.0)
_P3 = (0.0, H)


def bezier(t: float):
    mt = 1.0 - t
    r = (mt ** 3 * _P0[0] + 3 * mt * mt * t * _P1[0] + 3 * mt * t * t * _P2[0])
    z = (mt ** 3 * _P0[1] + 3 * mt * mt * t * _P1[1]
         + 3 * mt * t * t * _P2[1] + t ** 3 * _P3[1])
    return r, z


_TS = np.linspace(0.0, 1.0, 400)
_RS = np.array([bezier(t)[0] for t in _TS])
_ZS = np.array([bezier(t)[1] for t in _TS])


def radius_at(z: float) -> float:
    return float(np.interp(z, _ZS, _RS))


def body_front(x: float, z: float):
    """Y de la cara frontal de la gota en (x, z)."""
    r = radius_at(z)
    if r <= 1e-6:
        return None
    q = min(1.0, abs(x) / r)
    return -r * SQUASH * math.sqrt(max(0.0, 1.0 - q * q))


def body_back(x: float, z: float):
    r = radius_at(z)
    if r <= 1e-6:
        return None
    q = min(1.0, abs(x) / r)
    return r * SQUASH * math.sqrt(max(0.0, 1.0 - q * q))


def drop_mesh(segments=72, rings=64):
    """Gota con base plana: anillos de la Bezier + tapa inferior + punta."""
    verts = []
    for k in range(rings):
        r, z = bezier(k / rings)
        for j in range(segments):
            a = 2 * np.pi * j / segments
            verts.append([r * math.cos(a), r * math.sin(a) * SQUASH, z])
    verts = np.asarray(verts, float)
    bottom = len(verts)
    verts = np.vstack([verts, [[0, 0, 0]]])
    top = len(verts)
    verts = np.vstack([verts, [[0, 0, H]]])

    faces = []
    for k in range(rings - 1):
        base, nxt = k * segments, (k + 1) * segments
        for j in range(segments):
            a, b = base + j, base + (j + 1) % segments
            c, d = nxt + (j + 1) % segments, nxt + j
            faces += [(a, b, c), (a, c, d)]
    for j in range(segments):
        faces.append((bottom, (j + 1) % segments, j))
    last = (rings - 1) * segments
    for j in range(segments):
        faces.append((last + j, last + (j + 1) % segments, top))
    return verts, np.asarray(faces, np.int32)


def tube(points, radius, nu=12):
    verts, faces = [], []
    offset = 0
    for p in points:
        v, f = superellipsoid(p, (radius, radius, radius), 1.0, 1.0, nu, nu // 2)
        faces.append(f + offset)
        verts.append(v)
        offset += len(v)
    return np.vstack(verts), np.vstack(faces)


# ---------------------------------------------------------------- model
def model():
    parts = []

    def add(geo, color):
        parts.append((geo[0], geo[1], np.asarray(color, float)))

    add(drop_mesh(), YELLOW)

    # alas = las dos paletas del agarre
    for sx in (-1, 1):
        add(superellipsoid((sx * (radius_at(78) + 2), -3, 78), (13, 7, 17), 0.85, 0.85), WHITE)

    # ojos (izquierdo = MLX, derecho = LED) -- esferas achatadas, no discos:
    # asi la curvatura del cuerpo no se los come.
    for sx in (-1, 1):
        y = body_front(sx * 16, 90)
        if y is not None:
            add(superellipsoid((sx * 16, y - 1.0, 90), (7.0, 4.6, 7.0), 1.0, 1.0, 28, 16), NAVY)
            if sx < 0:
                add(superellipsoid((sx * 16, y - 2.6, 90), (4.0, 3.0, 4.0), 1.0, 1.0, 22, 14), PORT)
            else:
                add(superellipsoid((sx * 16, y - 2.6, 90), (2.2, 2.6, 2.2), 1.0, 1.0, 18, 12), BLUSH)

    # mejillas
    for sx in (-1, 1):
        y = body_front(sx * 29, 78)
        if y is not None:
            add(superellipsoid((sx * 29, y - 1.0, 78), (5.2, 3.0, 5.2), 1.0, 1.0, 24, 14), BLUSH)

    # sonrisa: tubo apoyado en la superficie, un poco hacia afuera
    pts = []
    for k in range(-9, 10):
        a = k / 9.0 * 0.95
        x = 10.5 * math.sin(a)
        z = 70.0 + 6.0 * (1.0 - math.cos(a))
        y = body_front(x, z)
        if y is not None:
            pts.append([x, y - 1.1, z])
    if len(pts) > 2:
        add(tube(pts, 1.9), NAVY)

    # ventana del dedo (MAX30102) en la panza
    y = body_front(0, 45)
    if y is not None:
        add(superellipsoid((0, y - 0.8, 45), (10.0, 4.0, 10.0), 1.0, 1.0, 36, 18), RECESS)
        add(superellipsoid((0, y - 2.2, 45), (6.8, 3.4, 6.8), 1.0, 1.0, 32, 16), PORT)

    # USB-C atras
    yb = body_back(0, 34)
    if yb is not None:
        add(superellipsoid((0, yb - 0.5, 34), (6.0, 2.6, 3.2), 0.25, 0.25, 18, 10), PORT)

    return parts


def lathe(profile, segments=96, squash=SQUASH):
    """Revoluciona un perfil cerrado (r, z) alrededor del eje z."""
    pts = list(profile)
    verts = []
    for r, z in pts:
        for j in range(segments):
            a = 2 * np.pi * j / segments
            verts.append([r * math.cos(a), r * math.sin(a) * squash, z])
    verts = np.asarray(verts, float)
    faces = []
    n = len(pts)
    for i in range(n):
        nxt = (i + 1) % n
        for j in range(segments):
            a = i * segments + j
            b = i * segments + (j + 1) % segments
            c = nxt * segments + (j + 1) % segments
            d = nxt * segments + j
            faces += [(a, b, c), (a, c, d)]
    return verts, np.asarray(faces, np.int32)


def stand():
    """Base para que Pulsi se pare: disco con asiento hundido."""
    profile = [(0, 0), (56, 0), (58, 2), (58, 10), (40, 14), (34, 14), (34, 9), (0, 9)]
    verts, faces = lathe(profile, 96, squash=1.0)
    return [(verts, faces, np.asarray(YELLOW_DARK, float))]


# ---------------------------------------------------------------- renderer
def render(parts, forward, up, path, size=SIZE, ss=SS, center=(0, 0, 68),
           world_span=175.0):
    f = np.asarray(forward, float); f /= np.linalg.norm(f)
    up = np.asarray(up, float)
    r = np.cross(f, up); r /= np.linalg.norm(r)
    v = np.cross(r, f)

    w = size * ss
    scale = w / world_span
    c = np.asarray(center, float)

    img = np.ones((w, w, 3), np.float32)
    zbuf = np.full((w, w), np.inf, np.float32)

    light1 = -f * 0.75 + v * 0.48 - r * 0.28
    light1 /= np.linalg.norm(light1)
    light2 = -f * 0.55 - v * 0.25 + r * 0.55
    light2 /= np.linalg.norm(light2)

    for verts, faces, color in parts:
        p = verts - c
        sx = ((p @ r) * scale + w / 2).astype(np.float32)
        sy = (-(p @ v) * scale + w / 2).astype(np.float32)
        d = p @ f

        tri = verts[faces]
        n = np.cross(tri[:, 1] - tri[:, 0], tri[:, 2] - tri[:, 0])
        ln = np.linalg.norm(n, axis=1)
        good = ln > 1e-9
        n = n[good] / ln[good, None]
        flip = (n @ (-f)) < 0
        n[flip] *= -1
        lam = np.clip(n @ light1, 0, 1) * 0.55 + np.clip(n @ light2, 0, 1) * 0.18 + 0.36
        cols = np.clip(np.asarray(color, float)[None, :] * lam[:, None], 0, 255)

        for k, (a, b, cc) in enumerate(faces[good]):
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
            mask = ((w0 >= 0) & (w1 >= 0) & (w2 >= 0)) | (
                (w0 <= 0) & (w1 <= 0) & (w2 <= 0))
            if not mask.any():
                continue
            z = w0 * d[a] + w1 * d[b] + w2 * d[cc]
            sub = zbuf[miny:maxy + 1, minx:maxx + 1]
            better = mask & (z < sub)
            if not better.any():
                continue
            sub[better] = z[better]
            img[miny:maxy + 1, minx:maxx + 1][better] = cols[k]

    out = Image.fromarray(np.clip(img, 0, 255).astype(np.uint8))
    out.resize((size, size), Image.LANCZOS).save(path)
    return path


def write_stl(parts, path):
    tris = np.vstack([verts[faces] for verts, faces, _ in parts])
    with open(path, "w", encoding="ascii") as fh:
        fh.write("solid pulsi_case\n")
        for t in tris:
            nn = np.cross(t[1] - t[0], t[2] - t[0])
            ln = np.linalg.norm(nn)
            nn = nn / ln if ln > 0 else np.zeros(3)
            fh.write(f"  facet normal {nn[0]:.5f} {nn[1]:.5f} {nn[2]:.5f}\n    outer loop\n")
            for p in t:
                fh.write(f"      vertex {p[0]:.5f} {p[1]:.5f} {p[2]:.5f}\n")
            fh.write("    endloop\n  endfacet\n")
        fh.write("endsolid pulsi_case\n")


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
        print("wrote", render(parts, fwd, up, OUT / f"{name}.png"))
    print("wrote", render(stand(), (0.7, 1.0, -0.55), (0, 0, 1), OUT / "stand.png",
                          center=(0, 0, 8), world_span=150.0))
    write_stl(parts, OUT.parent / "blockout.stl")
    print("wrote blockout.stl")
