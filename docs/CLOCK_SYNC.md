# Synchronizing the eBadge clock

The time page and anniversary calculations require a valid system clock.
The host helper uses the NSH `date -u -s` command to set UTC. The application
adds its configured display offset (UTC+08:00 by default); do not write local
Shanghai time as UTC or the display will be eight hours ahead.

From the team repository, preview without opening any device:

```sh
python3 tools/sync_board_clock.py
```

To synchronize explicitly:

```sh
python3 tools/sync_board_clock.py --port /dev/ttyUSB0 --apply
```

Requirements: Python 3, pyserial, the Huangshan Pi CH340 serial connection,
serial read/write permission and firmware with the NSH date command enabled.
Close other serial monitors and flash tools first. Host clock accuracy is
the user's responsibility; no internet time service is queried.

Opening serial can reset this board, losing RAM-only settings. The helper
waits for boot and uses the NSH console; it does not start another eBadge
instance, alter charging registers, format storage or flash firmware.
Use the autostart firmware so the interface returns after a reset.

The helper only reports success after reading board UTC back and finding it
within five seconds of host UTC. No readable response or excessive logging
is an error, not evidence that the clock was synchronized. A failure after
the set command may still leave the clock changed.

Status: helper implemented, not executed or validated on hardware yet.
This does not implement a touch-screen clock editor, Bluetooth clock sync,
or establish RTC retention across total power loss.
