---
name: huangshan-bringup
description: Prepare and diagnose an openvela Huangshan Pi baseline build, serial bring-up, and evidence capture using the contest manifest. Use for this board's toolchain, build, or flashing workflow, not generic UI development.
---

# Huangshan Pi bring-up

Use the team's actual manifest and pinned BSP, not SiFli SDK or RT-Thread examples as the final runtime. Retain app/hello_app mapping. Read the project's ENVIRONMENT and STATUS records first; historical Windows paths may differ from the current Linux host.

## Preflight

Run `python3 scripts/preflight.py --workspace /absolute/openvela` from this skill directory. It prints JSON without opening serial devices, changing configuration, reading credentials, or starting downloads. Source the project's local tool environment first if present. The report is a point-in-time prerequisite check, not a firmware or hardware test.

An initialized .repo does not prove sync completed. Require successful repo sync and inspect its exit code. If sync appears quiet, check the actual process handle and current fetch processes before restarting. Use no force-sync and retain local changes. If transport fails, inspect the error; use the documented mirror when appropriate without disabling TLS/signature validation. Avoid replacing all remote URLs blindly.

Current official Ubuntu quickstart documents 22.04 and 40 GB free/16 GB RAM. Other host versions and NTFS mounts require actual compatibility tests, not an automatic success or failure claim. Probe case sensitivity, symlinks and execution in a disposable directory before placing builds there. Prefer project-local tool packages when system installation is unavailable; verify packages against the distribution index and record exact versions.

## Baseline build

Read the synchronized `vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/README_zh-cn.md` and its `configs/nsh/defconfig`. Inspect matching NuttX/driver declarations before changing configuration. Check tool versions against that revision. Record a locked `repo manifest -r` after sync succeeds.

The documented CMake entry, from the openvela workspace root, is:

```bash
cmake -B cmake_out/lckfb_huangshan_pi -S "$PWD/nuttx" -GNinja \
  -DBOARD_CONFIG=../vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/configs/nsh \
  -DEXTRA_FLAGS="-Wno-cpp -Wno-deprecated-declarations"
cmake --build cmake_out/lckfb_huangshan_pi
```

Check this against the checked-out README before using it. Preserve build stdout/stderr and exit code. Do not mask a failing build with tee's successful exit. Record nuttx.bin SHA256, configuration and source revision. A compiler smoke test is not a baseline build.

## Serial and flashing

The board's CH340N RTS drives reset. Merely opening a port with default settings can keep the board reset. Check actual device, permissions and existing ownership first; use a tool with explicit RTS/DTR control. Documented baseline is 1,000,000 baud, 8N1, no flow control. Do not guess that every USB serial port is this board.

Confirm existing authorization before a flash. If absent, present the exact verified firmware, hash, port and intended operation for approval. Use installed sftool's help and the pinned BSP's documented address (baseline 0x12010000); do not guess an address or erase calibration/entire flash. Existing permission is not requested again. No automatic power/charging register experiments.

After an authorized flash, capture actual NSH boot and uname, framebuffer/LVGL display, touch, buttons and heap output. Treat IMU, network, charging and deep sleep as independent capabilities requiring driver and hardware evidence. A black display does not prove low power. Record failures, not expected results.

## Evidence

Write environment checks, build exit, source/config/hash and hardware observations to the project's docs. Label host tests separately. Development-Skill use is distinct from loading a runtime ai_agent Skill. A generated Skill or successful preflight alone does not complete DEV-02/TC-19; preserve its actual use record and later build/device evidence.

Official references:
- https://github.com/open-vela/vendor_sifli/blob/dev-ai-contest-2026/boards/sf32lb52/lckfb_huangshan_pi/README_zh-cn.md
- https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/quickstart/openvela_ubuntu_quick_start.md
