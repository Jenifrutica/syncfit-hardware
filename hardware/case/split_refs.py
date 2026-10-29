"""Split the Pulsi reference sheet into individual views.

Source: ~/Downloads/pulsito.jpeg (2048 x 2048, five renders on pastel
backgrounds). The five bodies are detected by their yellow, so each crop
follows its figure instead of a fixed grid; the padding adds the wings and the
feet and stays clear of the labels and the neighbours.

    python3 hardware/case/split_refs.py [origen] [destino]
"""

from __future__ import annotations

import sys
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage

SRC = Path.home() / "Downloads" / "pulsito.jpeg"
DST = Path(__file__).resolve().parent / "views"
SIZE = 768

ANCHORS = {
    "ref_front": (0.25, 0.27),
    "ref_threequarter": (0.75, 0.27),
    "ref_profile": (0.50, 0.52),
    "ref_back": (0.25, 0.74),
    "ref_top": (0.75, 0.74),
}

PAD_X = 0.11
PAD_Y = 0.05

# restos de vecinos o de las etiquetas: (x0, y0, x1, y1) en fraccion del recorte
CLEAN = {
    "ref_threequarter": [(0.00, 0.965, 1.00, 1.00)],
    "ref_profile": [(0.00, 0.00, 0.20, 0.09), (0.80, 0.00, 1.00, 0.09)],
    "ref_back": [(0.82, 0.00, 1.00, 0.10)],
    "ref_top": [(0.00, 0.00, 0.14, 0.12)],
}


def body_mask(rgb: np.ndarray) -> np.ndarray:
    r = rgb[..., 0].astype(int)
    g = rgb[..., 1].astype(int)
    b = rgb[..., 2].astype(int)
    return (r - b > 45) & (r > 140) & (g > 100)


def main() -> None:
    src = Path(sys.argv[1]) if len(sys.argv) > 1 else SRC
    dst = Path(sys.argv[2]) if len(sys.argv) > 2 else DST
    dst.mkdir(parents=True, exist_ok=True)

    sheet = Image.open(src).convert("RGB")
    rgb = np.asarray(sheet)
    h, w, _ = rgb.shape

    mask = body_mask(rgb)
    labels, count = ndimage.label(mask, structure=np.ones((3, 3)))
    sizes = ndimage.sum(mask, labels, range(1, count + 1))
    slices = ndimage.find_objects(labels)

    bodies = []
    for label in range(1, count + 1):
        if sizes[label - 1] < 0.01 * h * w or slices[label - 1] is None:
            continue
        sl = slices[label - 1]
        cx = (sl[1].start + sl[1].stop) / 2 / w
        cy = (sl[0].start + sl[0].stop) / 2 / h
        bodies.append((label, cx, cy, sl))

    used = set()
    for name, (ax, ay) in ANCHORS.items():
        best, best_d = None, 1e18
        for i, (label, cx, cy, sl) in enumerate(bodies):
            if label in used:
                continue
            d = (cx - ax) ** 2 + (cy - ay) ** 2
            if d < best_d:
                best, best_d = i, d
        if best is None:
            raise SystemExit("no encontre la figura " + name)
        label, cx, cy, sl = bodies[best]
        used.add(label)

        x0, x1 = sl[1].start, sl[1].stop
        y0, y1 = sl[0].start, sl[0].stop
        px = int((x1 - x0) * PAD_X)
        py = int((y1 - y0) * PAD_Y)
        crop = sheet.crop((max(0, x0 - px), max(0, y0 - py),
                           min(w, x1 + px), min(h, y1 + py)))
        for cx0, cy0, cx1, cy1 in CLEAN.get(name, []):
            box = (int(cx0 * crop.width), int(cy0 * crop.height),
                   int(cx1 * crop.width), int(cy1 * crop.height))
            pad = 12
            ring = crop.crop((max(0, box[0] - pad), max(0, box[1] - pad),
                              min(crop.width, box[2] + pad), min(crop.height, box[3] + pad)))
            colour = tuple(int(v) for v in np.median(np.asarray(ring).reshape(-1, 3), axis=0))
            crop.paste(Image.new("RGB", (box[2] - box[0], box[3] - box[1]), colour), box[:2])
        side = max(crop.size)
        square = Image.new("RGB", (side, side), (255, 255, 255))
        square.paste(crop, ((side - crop.width) // 2, (side - crop.height) // 2))
        square.resize((SIZE, SIZE), Image.LANCZOS).save(dst / f"{name}.png")
        print(f"{name}: {x1 - x0}x{y1 - y0} -> {crop.size}")


if __name__ == "__main__":
    main()
