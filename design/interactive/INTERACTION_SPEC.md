# Rectangular eBadge interaction prototype

Status: browser prototype only; no firmware migration or board acceptance.
Canvas: 390x450, uniformly scaled for smaller browser viewports. No round mask.
The three existing original PNGs are reused unchanged. Browser fonts are not
proof of equivalent font coverage or memory usage in the embedded build.

## Navigation contract

- Clean home: portrait, small name/caption and decorative motif, no visible menu.
- Hold for 600 ms with movement no greater than 10 logical pixels: open menu.
- Upward swipe over 60 pixels, vertical/horizontal ratio over 1.35: open menu.
- Horizontal swipe over 55 pixels with dominance over 1.35: cycle characters.
- Touch movement beyond 10 pixels cancels hold. A triggered hold consumes release.
- Lost/cancelled input, multiple pointers, focus loss and hidden document cancel
  an in-flight home gesture. No home gestures while a menu or editor is open.
- A short tap produces only an 850 ms character-specific ornament response.
- Menu backdrop or explicit close dismisses it. Functional pages have a back
  button; editor back discards the draft and returns to the originating page.
- Keyboard: Enter/Up/Space opens home menu, Left/Right changes character,
  Escape closes menu or returns. Menu cycles keyboard focus internally.

## Implemented browser routes

Home, hidden menu, clock, scrolling message, anniversary, appearance,
message editor and date editor. Menu has only clock/message/anniversary/appearance.
Character art, motifs and page colors derive from one character configuration.
Clock uses host time displayed in Asia/Shanghai. Anniversary uses calendar-date
differences in that zone, not a rounded interval from current time to midnight.
Dates are validated in 2024..2099. Text is nonblank and below 160 UTF-8 bytes.
Text is inserted as text, never interpreted as HTML. No date is fabricated for
an empty anniversary. Reduced-motion mode disables marquee and permits manual
text scrolling instead.

Preferences are stored ONLY in this browser under ebadge.rectangular-preview.v1.
Storage failures are shown outside the simulated display. No board filesystem,
clock, brightness register, charging setting or physical device is modified.
Night theme changes UI colors, not artwork or hardware brightness.

## Board migration still required

Translate this navigation state machine and rendering to existing LVGL pages,
route menu selections to the existing page/editor APIs, replace the old vertical
page cycling with menu navigation, and gate all home gestures while overlays
are active. Use actual LVGL display dimensions to scale from the 390x450 design.
Embedded character/font assets must be generated and resource usage measured.
Browser localStorage must not be represented as board persistence. Existing CLI
interfaces remain unchanged. Audio, LEDs, battery, Bluetooth, import and storage
work remain separate unfinished features, not simulated working menu items.

## Acceptance pending

Desktop and touch hold/swipe cancellation, multi-touch, release after menu-open,
menu dismissal/focus, all three characters, editor cancellation, UTF-8 limits,
leap dates, midnight/timezone boundaries, unavailable browser storage, narrow
viewports and reduced motion. Then separately compile and test LVGL on hardware;
this prototype cannot prove firmware behavior or display fidelity.
