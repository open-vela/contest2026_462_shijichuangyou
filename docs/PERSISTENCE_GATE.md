# Persistent settings storage gate

Observed on the connected Huangshan Pi, 2026-09-11. This is a device/storage
probe against the currently flashed English firmware, not a test of the new
Chinese UI build.

## Actual mounts

```text
/data type tmpfs
/etc type romfs
/proc type procfs
```

The writable /data mount is RAM-backed. Saving there does not survive reset.
The boot log also states that NOR write/erase is disabled during XIP bringup.
No flash erase, charger-register change, filesystem format or card write was
performed in this probe.

## Storage-card device

/dev/mmcsd0 is registered. A short, explicit single-sector read command failed:

```text
dd if=/dev/mmcsd0 of=/data/es.bin bs=512 count=1
nsh: dd: infd open failed: 5
```

The RAM output file was not created (`stat failed: 2`). Device presence alone
therefore does not prove usable media. EIO cannot distinguish absent media,
card/contact problems or driver initialization failure. Card insertion status
must be confirmed before choosing the next hardware diagnostic.

The first attempt used a longer command/path and encountered command splitting
and ENAMETOOLONG. Its incomplete command was not used as the final evidence;
the shorter command above reproduced the input-device open failure.

## Mount safety

The inspected NSH mount command forwards its -o string as filesystem data and
calls mount with flags zero. Therefore `mount -o ro` cannot be assumed to set
MS_RDONLY without inspecting the filesystem's option parser. No mount was
attempted under an unverified read-only assumption.

## Feature status

Text, anniversary and palette editors still correctly advertise RAM-only
settings. Persistent storage is not implemented or accepted. Do not change
those notices until an actual nonvolatile read/write and reboot test passes.
Images/audio stored on removable media share this dependency.

Next steps: confirm a card is present, diagnose media initialization, establish
a filesystem without formatting existing user data, then implement checked
settings load/save with corruption handling and power-cycle acceptance.

The temporary probe is finished and the existing ebadge application was
restarted; LCD and touchscreen initialization reported success. No new image
was flashed during this inspection. Other reference features remain open.
