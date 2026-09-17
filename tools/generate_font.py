#!/usr/bin/env python3
"""Regenerate the embedded Chinese fonts for the eBadge application.

The project ships its fonts as C source, so a firmware-only change to a label
can silently produce missing glyph boxes on the panel. This script makes the
generation reproducible and tools/verify_font_coverage.py turns a missing
glyph into a check failure instead of a runtime surprise.

Requirements:
  * Python 3 with fontTools (used to pull one face out of the Noto .ttc files;
    lv_font_conv cannot select a collection index itself)
  * lv_font_conv 1.5.x (Node). Pass --lv-font-conv, set LV_FONT_CONV, or put
    it on PATH.
  * Noto Sans CJK SC and Noto Serif CJK SC, from fonts-noto-cjk.

Usage:
  python3 tools/generate_font.py                 # regenerate both faces
  python3 tools/generate_font.py --face body     # only the 20 px sans face
  python3 tools/generate_font.py --dry-run       # report sizes, write nothing
"""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

import font_symbols

ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "app" / "hello_app" / "ui"

SANS_TTC = Path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc")
SERIF_TTC = Path("/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc")

# Face index 2 is "Noto Sans/Serif CJK SC" in the Ubuntu fonts-noto-cjk
# collection. generate_font.py verifies the name before using it.
SC_FACE_NAME = "CJK SC"
SC_FACE_INDEX = 2

FACES = {
    "body": {
        "ttc": SANS_TTC,
        "symbols": font_symbols.body_symbols,
        "size": 20,
        "bpp": 4,
        "name": "ebadge_font_zh_20",
        "output": "ebadge_font_zh_20.c",
        "face_label": "Noto Sans CJK SC",
    },
    "title": {
        "ttc": SERIF_TTC,
        "symbols": font_symbols.title_symbols,
        "size": 26,
        "bpp": 4,
        "name": "ebadge_font_title_26",
        "output": "ebadge_font_title_26.c",
        "face_label": "Noto Serif CJK SC",
    },
}


def find_lv_font_conv(explicit):
    if explicit:
        return explicit
    env = os.environ.get("LV_FONT_CONV")
    if env:
        return env
    found = shutil.which("lv_font_conv")
    if found:
        return found
    # Workspace-local install used by this project (.tools/npm).
    for parent in ROOT.parents:
        candidate = parent / ".tools" / "npm" / "node_modules" / ".bin" / "lv_font_conv"
        if candidate.is_file():
            return str(candidate)
    raise SystemExit(
        "lv_font_conv not found. Install it (npm install lv_font_conv) or pass "
        "--lv-font-conv / set LV_FONT_CONV."
    )


def extract_face(ttc, index, expected_label, cache_dir):
    """Write one .ttc face out as .otf and return its path."""
    from fontTools.ttLib import TTCollection

    out = Path(cache_dir) / (ttc.stem + "-face%d.otf" % index)
    if out.is_file():
        return out
    collection = TTCollection(str(ttc))
    if index >= len(collection.fonts):
        raise SystemExit("%s has no face %d" % (ttc, index))
    face = collection.fonts[index]
    label = str(face["name"].getDebugName(1))
    if expected_label not in label:
        raise SystemExit(
            "face %d of %s is %r, expected one containing %r"
            % (index, ttc.name, label, expected_label)
        )
    face.save(str(out))
    return out


def run_one(face, lv_font_conv, cache_dir, dry_run):
    spec = FACES[face]
    ttc = spec["ttc"]
    if not ttc.is_file():
        raise SystemExit("source font missing: %s" % ttc)
    symbols = spec["symbols"]()
    otf = extract_face(ttc, SC_FACE_INDEX, SC_FACE_NAME, cache_dir)
    output = UI / spec["output"]
    cmd = [
        lv_font_conv,
        "--font", str(otf),
        "--size", str(spec["size"]),
        "--bpp", str(spec["bpp"]),
        "--format", "lvgl",
        "--no-compress",
        "--no-kerning",
        "--lv-include", "lvgl/lvgl.h",
        "--lv-font-name", spec["name"],
        "--symbols", symbols,
        "-o", str(output),
    ]
    print("[%s] %s, %d symbols" % (face, spec["face_label"], len(symbols)))
    if dry_run:
        print("  dry-run: %s" % " ".join(cmd[:6]))
        return
    subprocess.run(cmd, check=True)
    print("  wrote %s (%d bytes)" % (output.relative_to(ROOT), output.stat().st_size))


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--face", choices=sorted(FACES), action="append",
                        help="regenerate only this face (repeatable)")
    parser.add_argument("--lv-font-conv", help="path to the lv_font_conv binary")
    parser.add_argument("--cache-dir", help="where extracted .otf faces are cached")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    lv_font_conv = find_lv_font_conv(args.lv_font_conv)
    faces = args.face or sorted(FACES)
    cache_dir = args.cache_dir or tempfile.mkdtemp(prefix="ebadge-fonts-")
    Path(cache_dir).mkdir(parents=True, exist_ok=True)
    for face in faces:
        run_one(face, lv_font_conv, cache_dir, args.dry_run)
    if not args.dry_run:
        print("done. run tools/verify_font_coverage.py to check every literal.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
