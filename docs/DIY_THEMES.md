# On-device DIY palette selection

Added a fifth page after anniversary: on-device palette selection.
Vertical swipes cycle all five pages in both directions. A short stationary
tap on the DIY page cycles forest, warm daylight, and monochrome palettes.
Long presses and drags do not switch palettes. Horizontal character selection
remains on the companion page only.

Palette changes update the root, character colors, eyes/mouth, dots, labels,
and the other four page surfaces without recreating the object tree or
resetting the current character or interaction state. The warm palette is
light with dark labels; the monochrome palette uses equal RGB components.

The selection lives in RAM and resets when the application restarts. The UI
explicitly states this limitation. Persistent storage, arbitrary background
images, six full reference styles, and on-device text/date editors remain open.
This feature changes display colors only; it is not physical RGB lighting.

Embedded Chinese glyphs are regenerated for the added labels and ASCII range.
Hardware appearance and gesture acceptance remain pending. Existing clock
invalid-time font fallback and LVGL event warnings remain separately tracked.
