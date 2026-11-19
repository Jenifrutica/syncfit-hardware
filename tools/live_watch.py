#!/usr/bin/env python3
"""Watch what the backend receives from the prototype (GET /live).

    python3 tools/live_watch.py                 # production, 4 Hz, until Ctrl+C
    python3 tools/live_watch.py --seconds 30 --api http://localhost:8001

Prints the fast force channel (kg, loss %, age) and the last frame (pulse,
finger, temperature). With --seconds it ends with min / max / mean / std of
the force, which is the number to look at for TARE drift and hysteresis.
"""

from __future__ import annotations

import argparse
import json
import statistics
import time
import urllib.request


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--api", default="https://api.syncfit.jenilarper.dev")
    parser.add_argument("--hz", type=float, default=4.0)
    parser.add_argument("--seconds", type=float, default=0.0)
    args = parser.parse_args()

    url = f"{args.api.rstrip('/')}/api/v1/live"
    forces: list[float] = []
    start = time.monotonic()
    try:
        while not args.seconds or time.monotonic() - start < args.seconds:
            with urllib.request.urlopen(url, timeout=5) as response:
                body = json.load(response)
            force = body.get("force") or {}
            bio = ((body.get("frame") or {}).get("biomarkers")) or {}
            if force:
                forces.append(force["force_kg"])
            print(
                f"{time.monotonic() - start:6.1f}s  conectado={body.get('connected')!s:5}  "
                f"fuerza={force.get('force_kg', '—'):>6} kg  pérdida={force.get('loss_pct', '—'):>5} %  "
                f"edad={force.get('age_seconds', '—'):>5}s  |  pulso={bio.get('heart_rate_bpm', '—')}  "
                f"dedo={bio.get('finger_present', '—')}  temp={bio.get('object_temperature_c', '—')} °C",
                flush=True,
            )
            time.sleep(1 / args.hz)
    except KeyboardInterrupt:
        pass
    if len(forces) > 1:
        print(
            f"\nfuerza: n={len(forces)} min={min(forces):.2f} max={max(forces):.2f} "
            f"media={statistics.mean(forces):.2f} desv={statistics.pstdev(forces):.3f} kg"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
