# Embedded character portraits

Three original project portraits were generated with the built-in image
generation tool on 2026-09-11. The reference poster's existing character art
was not copied into these assets.

- Taitai: mint-haired rabbit-hood companion, moss cape, emerald eyes.
- Nuannuan: copper-haired cat-hood companion, apricot scarf, amber eyes.
- Yueyue: silver-blue-haired lunar navigator, crescent hair clip, navy cape.

The PNG originals are stored under assets/portraits, together with a generated
manifest of source dimensions, SHA256 hashes and packed sizes. Native image
generation conversation records remain the provenance for the prompts.

Run `python3 tools/generate_portraits.py` with Pillow to regenerate the LVGL 9
image descriptors. Each portrait is 208x208 ARGB8888 (BGRA bytes on this
little-endian target), 173056 bytes; total pixel data is 519168 bytes.
Original alpha is preserved, including antialiased edges. Pixel data is const
firmware data; no SD card or runtime PNG decoder is required. Actual renderer
working-memory and frame-rate measurements remain hardware acceptance items.

The companion page now selects the matching portrait and moves it with the
existing idle/reactive bob animation and greeting text. The old geometric
characters are retained but hidden, not yet exposed as a style selector.
The portraits themselves do not blink or change facial expression. This is
static portrait display plus object-position animation, NOT GIF or Live2D.

DIY palettes affect surrounding UI, not the original portrait colors. The
monochrome palette therefore does not promise grayscale artwork. User image
upload, arbitrary file-backed backgrounds, expression frames, full reference
styles and animated-image decoders remain open requirements.
