#!/usr/bin/env python3
"""Check that every Chinese character the UI can draw exists in the font.

The fonts are compiled-in C subsets. A label added without regenerating the
font renders as an empty box on the panel, and the failure is invisible in a
successful build. This check turns that into a build-time error.

It reads the generated lv_font_conv C output and reconstructs the cmap, then
scans the application sources for wide characters inside string literals.

Exit status is non-zero when a literal would draw a missing glyph.

Usage:
  python3 tools/verify_font_coverage.py
  python3 tools/verify_font_coverage.py --list-missing
"""

import argparse
import re
import sys
from pathlib import Path

import font_symbols

ROOT = Path(__file__).resolve().parents[1]
APP = ROOT / "app" / "hello_app"
BODY_FONT = APP / "ui" / "ebadge_font_zh_20.c"
TITLE_FONT = APP / "ui" / "ebadge_font_title_26.c"

SOURCE_SUFFIXES = (".c", ".h")
# The generated font sources are huge and obviously covered; skip them.
SKIP_NAMES = {"ebadge_font_zh_20.c", "ebadge_font_title_26.c",
              "ebadge_portraits.c", "ebadge_motion.c"}

CMAP_ENTRY = re.compile(
    r"\.range_start\s*=\s*(\d+)\s*,\s*\.range_length\s*=\s*(\d+)\s*,"
    r".*?\.unicode_list\s*=\s*(\w+)\s*,\s*"
    r"\.glyph_id_ofs_list\s*=\s*\w+\s*,\s*"
    r"\.list_length\s*=\s*(\d+)\s*,\s*\.type\s*=\s*(\w+)",
    re.S,
)
LIST_DECL = re.compile(r"static const uint16_t (\w+)\[\]\s*=\s*\{(.*?)\};", re.S)
CODEPOINT = re.compile(r"0[xX][0-9a-fA-F]+|\d+")


def parse_uint_list(body):
    return [int(token, 0) for token in CODEPOINT.findall(body)]


def font_coverage(path):
    """Return the set of codepoints a generated lv_font_conv file can render."""
    if not path.is_file():
        raise SystemExit("font source not found: %s\nrun tools/generate_font.py"
                         % path.relative_to(ROOT))
    text = path.read_text(encoding="utf-8", errors="replace")
    lists = {name: parse_uint_list(body) for name, body in LIST_DECL.findall(text)}
    covered = set()
    for start, length, list_name, list_length, kind in CMAP_ENTRY.findall(text):
        start, length = int(start), int(length)
        if "SPARSE" in kind:
            values = lists.get(list_name, [])
            if len(values) != int(list_length):
                raise SystemExit(
                    "%s: %s has %d entries, cmap declares %s"
                    % (path.name, list_name, len(values), list_length))
            covered.update(start + value for value in values)
        else:
            covered.update(range(start, start + length))
    if not covered:
        raise SystemExit("%s: no cmap entries parsed" % path.relative_to(ROOT))
    return covered


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    return text


STRING_LITERAL = re.compile(r'"((?:[^"\\\n]|\\.)*)"')

# lv_font_conv packs glyph offsets into a 20-bit field unless the LVGL build
# enables LV_FONT_FMT_TXT_LARGE. Exceeding it is a compile error, but only
# after the (very slow) generated file is compiled, so check it here instead.
BITMAP_INDEX_LIMIT = 1 << 20
BITMAP_INDEX = re.compile(r"\.bitmap_index = (\d+)")


def check_bitmap_limit(path):
    """Return (ok, largest_index) for LVGL's default 20-bit glyph offset."""
    text = path.read_text(encoding="utf-8", errors="replace")
    indices = [int(value) for value in BITMAP_INDEX.findall(text)]
    if not indices:
        raise SystemExit("%s: no glyph descriptors parsed" % path.relative_to(ROOT))
    largest = max(indices)
    print("  %-28s %d / %d" % (path.name + " bitmap_index", largest,
                               BITMAP_INDEX_LIMIT - 1))
    return largest < BITMAP_INDEX_LIMIT, largest


def wide_chars(text):
    for literal in STRING_LITERAL.findall(strip_comments(text)):
        for char in literal:
            if ord(char) > 0x7F:
                yield char


def collect_literals():
    """Map codepoint -> set of files that draw it."""
    found = {}
    for path in sorted(APP.rglob("*")):
        if path.suffix not in SOURCE_SUFFIXES or path.name in SKIP_NAMES:
            continue
        for char in wide_chars(path.read_text(encoding="utf-8", errors="replace")):
            found.setdefault(char, set()).add(str(path.relative_to(ROOT)))
    return found


def report(label, missing, list_missing):
    if not missing:
        print("  %-28s OK" % label)
        return True
    print("  %-28s MISSING %d glyph(s)" % (label, len(missing)))
    for char in sorted(missing):
        where = ", ".join(sorted(missing[char]))
        print("      U+%04X %s  <- %s" % (ord(char), char, where))
    if list_missing:
        print("      (regenerate with tools/generate_font.py)")
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--list-missing", action="store_true",
                        help="print only the missing characters")
    args = parser.parse_args()

    body = font_coverage(BODY_FONT)
    title = font_coverage(TITLE_FONT)
    literals = collect_literals()

    body_missing = {c: f for c, f in literals.items() if ord(c) not in body}
    title_missing = {c: {"tools/font_symbols.py TITLE_TEXT": None}
                     for c in font_symbols.TITLE_TEXT if ord(c) not in title}

    print("font coverage (%d literals, %d distinct wide chars)"
          % (sum(len(f) for f in literals.values()), len(literals)))
    print("  body  %s: %d codepoints" % (BODY_FONT.name, len(body)))
    print("  title %s: %d codepoints" % (TITLE_FONT.name, len(title)))
    body_fits, _ = check_bitmap_limit(BODY_FONT)
    title_fits, _ = check_bitmap_limit(TITLE_FONT)
    if not body_fits or not title_fits:
        print("\nFAILED: font exceeds LVGL's 20-bit glyph offset. Use a smaller")
        print("        glyph set (tools/font_symbols.py) or enable")
        print("        LV_FONT_FMT_TXT_LARGE for every font in the build.")
        return 1
    ok = report("body covers all literals", body_missing, args.list_missing)
    ok &= report("title covers title text", title_missing, args.list_missing)
    if not ok:
        print("\nFAILED: a literal would render as a missing glyph.")
        return 1
    print("\nOK: every wide character in the application has a glyph.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
