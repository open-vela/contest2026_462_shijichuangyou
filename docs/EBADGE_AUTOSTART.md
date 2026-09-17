# eBadge power-on startup

The Huangshan Pi board embeds its startup script in the firmware ROMFS.
The eBadge build must run `ebadge &` instead of the stock QuickApp launcher.
The existing three-second delay and background launch are retained, leaving
the NSH serial console available for recovery and diagnostics.

Local source change:
`vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/src/etc/init.d/rcS`

A matching patch is stored in
`board/contest_board/patches/0001-ebadge-autostart.patch`.
On a fresh, unmodified openvela checkout, apply it from the workspace root:

```sh
patch -p1 < contest2026_462_shijichuangyou/board/contest_board/patches/0001-ebadge-autostart.patch
```

Do not reapply to an already patched checkout. This patch is specific to the
eBadge configuration, which includes the `ebadge` executable. It replaces
the QuickApp launcher and is not intended for stock QuickApp firmware.

Rebuild and flash the resulting firmware for this change to reach hardware.
Editing the host script alone does not update the board's read-only ROMFS.
Do not launch a second `ebadge` instance manually after automatic startup.

Status: rebuilt and flashed with successful sftool write verification.
A serial-open reset produced automatic `ebadge` startup after about three
seconds and successful LCD/input initialization without any launch command.
True cold-power-on screen acceptance still requires user confirmation.
Evidence: `watch/docs/ebadge-autostart-flash-retry.log` and
`watch/docs/ebadge-autostart-boot-observation.log` outside the team repository.
LVGL input-event warnings remain unresolved. This local vendor change is not an
upstream submission; competition-required public-repository PR handling
remains outstanding. Keeping a patch in the team repo does not replace it.
