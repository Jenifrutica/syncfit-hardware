#!/usr/bin/env python3
"""Send serial commands to the SyncFit prototype and print what it answers.

Non-interactive replacement for `pio device monitor`, handy for scripted
calibration sessions:

    python3 tools/serial_cmd.py STATUS SCAN
    python3 tools/serial_cmd.py --wait 8 TARE
    python3 tools/serial_cmd.py --listen 20          # just read the log
    python3 tools/serial_cmd.py "SET WIFI MiRed,clave" REBOOT

Each command is sent with a newline; after it, the output is read for
--wait seconds (default 3). Opening the port resets most ESP32 boards, so
the script first waits --boot seconds for the firmware to start.
"""

from __future__ import annotations

import argparse
import glob
import sys
import time

import serial


def find_port() -> str:
    ports = sorted(glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*"))
    if not ports:
        sys.exit("No hay prototipo por USB (/dev/ttyUSB* o /dev/ttyACM*). Conecta el cable.")
    return ports[0]


def drain(port: serial.Serial, seconds: float) -> None:
    end = time.monotonic() + seconds
    while time.monotonic() < end:
        line = port.readline()
        if line:
            print(line.decode("utf-8", "replace").rstrip(), flush=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("commands", nargs="*")
    parser.add_argument("--port", default=None)
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--wait", type=float, default=3.0, help="seconds to read after each command")
    parser.add_argument("--boot", type=float, default=2.5, help="seconds to wait after opening the port")
    parser.add_argument("--listen", type=float, default=0.0, help="only read the log for N seconds")
    args = parser.parse_args()

    name = args.port or find_port()
    with serial.Serial(name, args.baud, timeout=0.2) as port:
        print(f"# {name} @ {args.baud}", flush=True)
        drain(port, args.boot)
        if args.listen:
            drain(port, args.listen)
        for command in args.commands:
            print(f"> {command}", flush=True)
            port.write((command + "\n").encode())
            port.flush()
            drain(port, args.wait)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
