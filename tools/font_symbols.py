#!/usr/bin/env python3
"""Glyph sets for the embedded eBadge fonts.

The sets are computed, not hand-copied, so they can be reproduced on any
machine: the ASCII range is fixed and the Chinese range is derived from the
Python GB2312 codec, which every CPython build ships. No external word list
is downloaded and no network access is required.

  body  - Noto Sans CJK SC. GB2312 level 1 (3755 common hanzi) plus CJK
          punctuation. This removes the old "only the poster vocabulary is
          embedded" limitation: UI copy, CLI --message text and preset phrases
          are all covered without regenerating the font.
  title - Noto Serif CJK SC. Only the heading glyphs actually drawn with the
          serif face, so the second face stays cheap in flash.

Glyph-set size is bounded by LVGL, not by flash. lv_font_conv emits a
20-bit glyph "bitmap_index" unless LV_FONT_FMT_TXT_LARGE is enabled, so the
whole font must stay below 1 MiB of bitmap data or the build fails with
"Too large font or glyphs". Level 1 (about 0.7 MiB at 20 px/4 bpp) fits the
default; adding GB2312 level 2 reaches 1.27 MiB and does NOT. Enabling the
LVGL Kconfig option would be a global change affecting every font, so the
default here stays at level 1.
"""

ASCII = "".join(chr(c) for c in range(0x20, 0x7F))

# GB2312 punctuation and symbols shipped as a contiguous block. These are the
# full-width forms a Chinese UI needs (、。《》【】etc.).
CJK_PUNCTUATION = (
    "\u3000\u3001\u3002\u00b7\u02c7\u02c9\u2014\uff5e\u2016\u2026\u2018\u2019"
    "\u201c\u201d\u3014\u3015\u3008\u3009\u300a\u300b\u300c\u300d\u300e\u300f"
    "\u3010\u3011\u00b1\u00d7\u00f7\u2236\u2227\u2228\u2211\u220f\u222a\u2229"
    "\u2208\u2237\u221a\u22a5\u2225\u2220\u2312\u2299\u222b\u222e\u2261\u224c"
    "\u2248\u223d\u221d\u2260\u226e\u226f\u2264\u2265\u221e\u2235\u2234\u2642"
    "\u2640\u00b0\u2032\u2033\u2103\uff04\u00a4\uffe0\uffe1\u2030\u00a7\u2116"
    "\u2606\u2605\u25cb\u25cf\u25ce\u25c7\u25c6\u25a1\u25a0\u25b3\u25b2\u203b"
    "\u2192\u2190\u2191\u2193\u3013\uff08\uff09\uff3b\uff3d\uff5b\uff5d\uff0c"
    "\uff0e\uff1a\uff1b\uff1f\uff01\uff1c\uff1e\uff02\uff03\uff05\uff06\uff0a"
    "\uff0b\uff0d\uff1d\uff20\uffe5"
)

# Full-width digits and Latin letters (GB2312 row 3) so a Chinese-styled label
# can use them without falling back to a half-width face.
FULLWIDTH = "".join(
    chr(c) for c in list(range(0xFF10, 0xFF1A)) + list(range(0xFF21, 0xFF3B))
    + list(range(0xFF41, 0xFF5B))
)


def gb2312_hanzi(level=2):
    """Return the GB2312 hanzi decoded through the stdlib codec.

    level 1 is the 3755 most common characters (rows 16..55); level 2 adds the
    3008 less common ones (rows 56..87). Characters the codec cannot map are
    skipped instead of raising.
    """
    if level not in (1, 2):
        raise ValueError("level must be 1 or 2")
    # GB2312 qu (区) numbers 16..55 hold level 1 and 56..87 hold level 2, but
    # the encoded lead byte is qu + 0xA0, so the byte range is 0xB0..0xD7 and
    # 0xD8..0xF7 respectively.
    last_byte = 0xD7 if level == 1 else 0xF7
    out = []
    for row in range(0xB0, last_byte + 1):
        for cell in range(0xA1, 0xFF):
            try:
                char = bytes((row, cell)).decode("gb2312")
            except UnicodeDecodeError:
                continue
            out.append(char)
    return "".join(out)


# Strings the UI draws with the serif face. Keep this list in sync with the
# heading literals in the source; tools/verify_font_coverage.py fails the build
# style check if a heading character is missing from the generated title font.
TITLE_TEXT = (
    "拾迹创游"
    "随身小册"
    "收起"
    "时钟"
    "文字应援"
    "纪念日"
    "配色"
    "与你共度"
    "把喜欢说出来"
    "值得记住的日子"
    "你的收藏"
    "苔苔"
    "暖暖"
    "月月"
    "把喜欢，藏进春天。"
    "与你分享，今天的暖阳。"
    "把心愿，寄给今晚的月亮。"
)


def body_symbols(level=1):
    """Deduplicated body-font symbol string, ASCII first for a dense cmap.

    level=1 is the default because level 2 pushes the generated bitmap past
    LVGL's default 20-bit glyph index limit; see the module docstring.
    """
    seen = {}
    for char in ASCII + CJK_PUNCTUATION + FULLWIDTH + gb2312_hanzi(level):
        seen[char] = None
    return "".join(seen)


def title_symbols():
    seen = {}
    for char in ASCII + TITLE_TEXT:
        seen[char] = None
    return "".join(seen)


if __name__ == "__main__":
    import sys

    which = sys.argv[1] if len(sys.argv) > 1 else "body"
    text = body_symbols() if which == "body" else title_symbols()
    sys.stdout.write(text)
    print("\n# %d symbols" % len(text), file=sys.stderr)
