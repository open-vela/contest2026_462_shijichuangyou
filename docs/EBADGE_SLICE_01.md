# eBadge first functional slice

Status: IMPLEMENTED_NOT_COMPILED_NOT_HARDWARE_TESTED.

The owner requested environment work and feature development in parallel.
This is preparatory implementation, not completion of the T02/T03 hardware
gates or permission to drop any P0/P1 requirements.

## Environment progress

- The locked LVGL source is locally checked out; lvgl.h identifies 9.1.0.
- The application follows apps/examples/lvgldemo/lvgldemo.c and the actual
  NuttX port headers, including the configured libuv event loop.
- The host savedefconfig launcher was added because the installed Ubuntu
  kconfiglib package supplies the Python module but not that executable.
- Existing staged NuttX deletions have NOT been restored or overwritten.
- No firmware build, unit test, or board operation was run for this slice.

## Implemented source

- app/hello_app/ebadge_main.c: separate ebadge executable, one LVGL owner,
  explicit display/input initialization failures, no network startup dependency.
- core/ebadge_controller.*: three-way cyclic navigation, 1200 ms reaction,
  repeated-tap suppression, callback generation/token guards, bounded queue
  with a navigation overflow mailbox and observable overflow counter.
- ui/ebadge_view.*: three original procedural study characters, responsive
  reference canvas, idle bob/blink and reaction motion; fixed object allocation
  at startup, no per-frame image decoding or object creation.
- One press/move/release recognizer: <=300 ms / <=12 px tap; >=45 px horizontal
  swipe with 1.5 direction ratio; no duplicate CLICKED/GESTURE handler. A touch
  that moves away and returns is not treated as a tap.
- Kconfig, CMake and Make routes include the new application. The old hello
  sample remains available; its legacy configuration symbol is retained.

## Intended board usage after the baseline is repaired

Enable LVX_USE_DEMO_CONTEST2026_462_EBADGE in menuconfig and rebuild.
Run `ebadge` from NSH, not alongside another LVGL application. This does not
yet replace the boot entrypoint. Stack size is a 16384-byte design default,
not measured stack sufficiency. Touch path defaults to /dev/input0.

## Resource origin and limits

MOSS, EMBER and LUNA are provisional, code-drawn study characters authored
for this implementation under Apache-2.0. There are no imported brand/IP
images, raster frames, special SiFli image formats, or external fonts.
ASCII labels intentionally avoid an unverified Chinese font dependency.
Final character selection, licensed Chinese font, asset inventory/hashes and
visual acceptance remain pending. A 33 ms timer is a requested cadence, not
a measured display frame rate.

## Required next checks (NOT_RUN)

Build/link both enabled and disabled app configurations; original hello
regression; first launch and initialization failure; 100 cyclic swipes;
tap vs. swipe/long press/move-return; switching during reaction; late and
duplicate animation completion; tick wrap; queue overflow; repeated taps;
touch cancellation; memory/stack measurement and a 30-minute animation run.

## Still pending in the full specification

Custom boot entry, real display dim/off/key wake, sensor input, bounded
hardware feedback, battery/settings, measured system power, wearable shell,
board ai_agent/real LLM/runtime Skill/proactive execution/offline fallback,
submission-approved development logs, reproducible release and final materials.
The queue is UI-task-only; future concurrent producers need synchronized
ingress, and power/wake priority must be added before enabling those sources.
No hardware or AI capability is represented by a fake success result.
