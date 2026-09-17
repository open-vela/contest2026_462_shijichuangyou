#!/usr/bin/env python3
"""Run NSH commands on the Huangshan Pi over the CH340 serial console.

Used for hardware verification that must leave reproducible evidence: SD card
probing, device enumeration and post-flash smoke checks. It only reads and
writes through the existing NSH console; it never flashes, erases or unmounts
anything on its own.

Opening the serial port resets this board, so every invocation waits for the
shell prompt after boot.

Access: the port is usually root:dialout. Either join the dialout group, or run
this script with sudo. Do not chmod the device node permanently.

Examples:
  python3 tools/board_nsh.py --wait 8 'ls /dev'
  python3 tools/board_nsh.py --script tools/probes/sdcard.txt
  python3 tools/board_nsh.py --echo 'uname -a'
"""

import argparse
import sys
import time
from pathlib import Path

DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200
PROMPT = b"nsh> "


def read_until(port, deadline, marker=None):
    """Collect bytes until the deadline, or until marker is seen."""
    chunks = []
    while time.monotonic() < deadline:
        waiting = port.in_waiting
        if waiting:
            data = port.read(waiting)
            if data:
                chunks.append(data)
                if marker and marker in data:
                    break
                continue
        time.sleep(0.03)
    return b"".join(chunks)


def clean(data, strip_ansi=True):
    if strip_ansi:
        import re

        data = re.sub(rb"\x1b\[[0-9;?]*[a-zA-Z]", b"", data)
    return data.decode("utf-8", errors="replace").replace("\r\n", "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("commands", nargs="*",
                        help="NSH commands to run, in order")
    parser.add_argument("--port", default=DEFAULT_PORT)
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--wait", type=float, default=6.0,
                        help="seconds to wait for the shell prompt after reset")
    parser.add_argument("--timeout", type=float, default=10.0,
                        help="seconds to wait for each command's output")
    parser.add_argument("--script", help="file with one command per line (# comments)")
    parser.add_argument("--echo", action="store_true",
                        help="echo each command and its raw output")
    parser.add_argument("--raw", action="store_true", help="keep ANSI escapes")
    args = parser.parse_args()

    commands = list(args.commands)
    if args.script:
        for line in Path(args.script).read_text(encoding="utf-8").splitlines():
            line = line.strip()
            if line and not line.startswith("#"):
                commands.append(line)
    if not commands:
        parser.error("no commands given")

    try:
        import serial
    except ImportError:
        raise SystemExit("pyserial is required: python3 -m pip install pyserial")

    try:
        port = serial.Serial(args.port, args.baud, timeout=0.2)
    except PermissionError:
        raise SystemExit(
            "permission denied on %s. Add your user to the dialout group, or "
            "run with sudo." % args.port)
    except serial.SerialException as exc:
        raise SystemExit("cannot open %s: %s" % (args.port, exc))

    with port:
        port.reset_input_buffer()
        boot = read_until(port, time.monotonic() + args.wait, PROMPT)
        if not args.raw and boot:
            print("--- boot ---")
            print(clean(boot).strip()[-1500:])
        for command in commands:
            port.reset_input_buffer()
            if args.echo:
                print("\nnsh> " + command)
            port.write((command + "\n").encode("utf-8"))
            output = read_until(port, time.monotonic() + args.timeout, PROMPT)
            text = clean(output, strip_ansi=not args.raw)
            # Drop the echoed command line and the trailing prompt.
            lines = text.split("\n")
            if lines and command in lines[0]:
                lines = lines[1:]
            print("\n".join(line for line in lines).rstrip())
    return 0


if __name__ == "__main__":
    sys.exit(main())
