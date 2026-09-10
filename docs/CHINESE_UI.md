# Chinese UI

The labels, character names, hints and greetings use Simplified Chinese.
The existing layout and interaction logic are unchanged.

Font: Noto Sans CJK SC Regular, derived from Ubuntu fonts-noto-cjk.
License: SIL Open Font License 1.1; see FONT_LICENSE.txt.
The generated ebadge_font_zh_20.c contains only the required glyphs at
20 px, 4 bits per pixel, without compression or kerning. It is compiled
into the application and does not require a filesystem font loader.

Generation tools: fontTools 4.46.0 and lv_font_conv 1.5.3.
Select the NotoSansCJKsc-Regular face from NotoSansCJK-Regular.ttc and
export it as OTF using fontTools.ttLib.TTFont.save, then run:

```sh
lv_font_conv --font NotoSansCJKsc-Regular.otf --size 20 --bpp 4 \
  --format lvgl --no-compress --no-kerning --lv-include lvgl/lvgl.h \
  --lv-font-name ebadge_font_zh_20 \
  --symbols '苔暖月送你一份小的快乐。有在，今天更温慢来每步都算数轻点打招呼左右滑动换伙伴拾迹创游 / 随身本地互中文版' \
  -o app/hello_app/ui/ebadge_font_zh_20.c
```

Previous English firmware: compiled, flashed and programmer verification passed.
The user reported its existing interactions working on the device.
Chinese version: source change only; build and hardware acceptance pending.
The previous LVGL event warning and lack of automatic app startup remain.

AI log audit: the team logs directory currently contains only the scaffold
example for your-github-login, not actual development sessions. Earlier
official collector 1.3.0 tests produced zero Codex events. A local adapter
passing schema validation is not evidence of organizer acceptance or upload.
No claim of official recording is made. No logs were fabricated or uploaded.
