#!/usr/bin/env python3
"""Set the Huangshan Pi NSH system clock from host UTC (explicit opt-in)."""
import argparse
from datetime import datetime, timezone
import re
import sys
import time


MONTHS = 'Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec'.split()
ANSI = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')


def set_command(now):
    if not 2024 <= now.year <= 2099:
        raise RuntimeError('Host year is outside the supported UI range.')
    value = (f'{MONTHS[now.month - 1]} {now.day:02d} '
             f'{now:%H:%M:%S} {now.year:04d}')
    return f'date -u -s "{value}"'


def receive(port, seconds, prompt=False):
    end = time.monotonic() + seconds
    data = bytearray()
    while time.monotonic() < end:
        data.extend(port.read(4096))
        if len(data) > 262144:
            raise RuntimeError('Excessive serial output; stop noisy logging first.')
        text = ANSI.sub('', data.decode('utf-8', errors='replace'))
        if prompt and 'nsh> ' in text:
            return text
    text = ANSI.sub('', data.decode('utf-8', errors='replace'))
    if prompt:
        raise RuntimeError('No NSH prompt received; no further command sent.')
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/ttyUSB0')
    parser.add_argument('--apply', action='store_true',
                        help='Open serial (may reset the board) and set time')
    args = parser.parse_args()
    print('Preview:', set_command(datetime.now(timezone.utc)))
    if not args.apply:
        print('No serial access. Add --apply to synchronize; opening may reset the board.')
        return 0

    import serial
    from serial.tools import list_ports
    from pathlib import Path

    target = Path(args.port).resolve()
    devices = [p for p in list_ports.comports()
               if Path(p.device).resolve() == target]
    if len(devices) != 1 or (devices[0].vid, devices[0].pid) != (0x1A86, 0x7523):
        raise RuntimeError('Selected port is not the expected CH340 USB adapter.')
    port = serial.Serial(port=None, baudrate=1000000, timeout=0.2,
                         write_timeout=2, exclusive=True)
    port.rts = False
    port.dtr = False
    port.port = args.port
    port.open()
    try:
        # Opening this board's serial adapter can reset it. Wait for auto-boot.
        receive(port, 6)
        port.write(b'\r\n')
        receive(port, 3, prompt=True)
        command = set_command(datetime.now(timezone.utc))
        port.write((command + '\r\n').encode('ascii'))
        reply = receive(port, 3, prompt=True)
        if 'nsh: date:' in reply:
            raise RuntimeError('NSH rejected the date command: ' + reply[-1200:])
        port.write(b'date -u "+%Y-%m-%dT%H:%M:%S"\r\n')
        reply = receive(port, 2)
        dates = re.findall(r'\b\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}\b', reply)
        if not dates:
            raise RuntimeError('No readable date response; synchronization unconfirmed.')
        board = datetime.strptime(dates[-1], '%Y-%m-%dT%H:%M:%S').replace(tzinfo=timezone.utc)
        delta = abs((datetime.now(timezone.utc) - board).total_seconds())
        if delta > 5:
            raise RuntimeError(f'Readback differs from host UTC by {delta:.1f}s.')
        print('Board UTC readback:', board.isoformat())
        print('Clock synchronized. Default eBadge display uses UTC+08:00.')
        print('Power-off RTC retention is not established by this operation.')
        return 0
    finally:
        port.close()


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Exception as exc:
        print('Clock synchronization failed:', exc, file=sys.stderr)
        sys.exit(1)
