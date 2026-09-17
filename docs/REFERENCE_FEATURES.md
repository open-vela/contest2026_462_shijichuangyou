# Reference-image implementation

Reference: user attachment image-1.png, conceptual electronic badge poster.
The objective remains the full requested reference, not just the first pages.

## Added software path (hardware acceptance pending)

- Existing three original characters and tap reactions are preserved.
- Vertical swipes cycle companion, clock, scrolling support text, anniversary.
- Horizontal swipes on the companion page retain character selection.
- Clock uses the device system clock with explicit UTC offset (default +480
  minutes). Dates outside 2024..2099 produce a request to set the clock.
- Support text scrolls using LVGL. CLI --message accepts up to 159 UTF-8 bytes.
- Anniversary uses --date YYYY-MM-DD, validates the actual Gregorian date,
  and shows days remaining, today, or days elapsed. No fabricated default date.
- CLI --tz-minutes accepts -720..840. Settings currently live in RAM only.
- The embedded Chinese font includes the default UI and poster vocabulary,
  not arbitrary user-supplied Chinese characters. A general-purpose text font
  and on-device editing remain required for unrestricted customization.

Example: ebadge --date 2026-09-20 --message "让喜欢发光" &
The date is an example supplied by the operator, not a claimed event date.

## Remaining reference requirements

- User images, GIF and live2D support: decoder/assets and memory budget needed.
- Audio/role voice/BGM: real audio device path, playback and controls needed.
- Support lights: actual RGB/PWM driver mapping and hardware tests needed.
- Battery/charging state: real power-driver data required, no fake percentage.
- Touch and shake reactions: touch exists; real IMU integration remains.
- DIY styles/backgrounds and message/date editors: not yet implemented.
- Bluetooth/NFC content transfer: hardware/stack feasibility and protocol needed.
- Wearable enclosure/battery operation: physical design and validation needed.
- Optional phone app, uploads and firmware update: scope/transport design needed.
- Original project sleep/wake and real board AI Agent requirements remain open.

No unsupported hardware capability is represented by a working-looking button.
No real-time audio, battery, light or connectivity claim follows from these pages.
The reference's copyrighted-looking character examples are not bundled as assets.
The existing LVGL event warning is still open; no claim of warning-free runtime.
