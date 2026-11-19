#!/usr/bin/env python3
"""Forward the prototype's telemetry from USB to the server.

The firmware prints every frame and force reading as `TX {json}` on the serial
port. This bridge sends them over the PC's own (reliable) connection, so the
demo works even when the ESP32's Wi-Fi does not. Run it and leave it open:

    python3 tools/usb_bridge.py            # /dev/ttyUSB0 -> production
"""
import argparse, asyncio, glob, json, time

import serial
import websockets

WS = "wss://api.syncfit.jenilarper.dev/api/v1/ws/telemetry"


async def run(port: str, url: str) -> None:
    ser = None
    sent = 0
    last_report = time.time()
    while True:
        try:
            async with websockets.connect(url, open_timeout=10, ping_interval=20, max_size=None) as ws:
                print(f"[bridge] connected to {url}", flush=True)

                async def drain():
                    async for _ in ws:
                        pass

                dev = port or (sorted(glob.glob("/dev/ttyUSB*")) or ["/dev/ttyUSB0"])[0]
                ser = serial.Serial(dev, 115200, timeout=0.05)
                print(f"[bridge] reading {dev}", flush=True)
                reader = asyncio.create_task(drain())
                buf = b""
                while not reader.done():
                    chunk = await asyncio.to_thread(ser.read, 4096)
                    buf += chunk
                    while b"\n" in buf:
                        raw, buf = buf.split(b"\n", 1)
                        line = raw.decode("utf-8", "replace").strip()
                        if not line.startswith("TX {"):
                            continue
                        payload = line[3:]
                        try:
                            json.loads(payload)
                        except ValueError:
                            continue  # line mixed with another print
                        await ws.send(payload)
                        sent += 1
                    if time.time() - last_report > 10:
                        print(f"[bridge] {sent} messages forwarded", flush=True)
                        last_report = time.time()
        except Exception as exc:  # network hiccup or USB replugged: retry
            if ser is not None:
                ser.close()
            print(f"[bridge] {type(exc).__name__}: {exc}; retrying in 2 s", flush=True)
            await asyncio.sleep(2)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", default=None)
    ap.add_argument("--url", default=WS)
    a = ap.parse_args()
    asyncio.run(run(a.port, a.url))
