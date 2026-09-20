#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""生成参赛提交材料：作品介绍文档（PDF/DOCX）、路演 PPT（PPTX）、演示视频脚本。

所有数字均取自本仓实测输出，不虚构：
  - 构建：ninja 退出码 0，0 warning
  - 体积：flash 9468380 B / 16 MB = 56.44%；SRAM 107180 B / 512 KB = 20.44%
  - 断言：tou 1622 项，0 失败
  - 字形：445 条字面量 / 282 个不同宽字符，0 缺失

用法：PYTHONPATH=<pylib> python3 tools/generate_submission_docs.py
依赖：reportlab（PDF）、python-docx、python-pptx
"""

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "submission"
OUT.mkdir(exist_ok=True)

TITLE = "拾迹创游 · 互动电子吧唧"
SUBTITLE = "可佩戴的二次元电子徽章"
TRACK = "AI 硬件产品创新"

def em_plain(s: str) -> str:
    """去掉 **强调** 标记（用于不支持富文本的目标）。"""
    return re.sub(r"\*\*(.+?)\*\*", r"\1", s)


def em_pdf(s: str) -> str:
    """把 **强调** 转成 reportlab 颜色标记。

    说明：嵌入的 AR PL UMing 没有粗体面（注册的 bold 指向同一字体），
    因此用主题色实现强调，而不是依赖加粗。
    """
    return re.sub(r"\*\*(.+?)\*\*", r'<font color="#FF6B9D">\1</font>', s)


CJK_NAME = "Noto Sans CJK SC"      # 供 docx/pptx 引用（在评委机器上渲染）
CJK_LATIN = "DejaVu Sans"           # 拉丁字形
PDF_FONT_CACHE = Path("/media/ran/EXTERNAL_USB/watch/.tools/fonts/uming-cn.ttf")


def ensure_pdf_font() -> str:
    """返回一个 reportlab 可用的中文 TrueType 字体文件路径。

    注意（这是踩过的坑）：不能用 DroidSansFallbackFull.ttf —— 它是纯 CJK
    回退字体，**不含任何拉丁字形**（连 A、o、0、/ 都没有），会让 "openvela"、
    "LVGL"、"390×450" 等全部渲染成空白。实测覆盖：
      DroidSansFallbackFull.ttf  缺 A o p 0 / + ×
      AR PL UMing CN（本函数）   仅缺 ● ⚠（已在正文中改用 ※ ●）
    reportlab 不支持 .ttc，也不支持 CFF/PostScript 轮廓，因此用 fontTools
    从 uming.ttc 中抽出单一字体面另存为 .ttf。
    """
    if PDF_FONT_CACHE.exists():
        return str(PDF_FONT_CACHE)
    from fontTools.ttLib import TTCollection
    src = "/usr/share/fonts/truetype/arphic/uming.ttc"
    PDF_FONT_CACHE.parent.mkdir(parents=True, exist_ok=True)
    TTCollection(src, lazy=False).fonts[0].save(str(PDF_FONT_CACHE))
    return str(PDF_FONT_CACHE)

# ---------------------------------------------------------------- 内容数据

ONELINER = (
    "一块可佩戴的电子吧唧：三位二次元角色陪着你看时间、留一句话、数纪念日，摇一摇会有反应。"
    "运行在 openvela / NuttX + LVGL 9.1，390×450 AMOLED，全中文界面，开机即用、不依赖手机与网络。"
)

PAIN = [
    ("徽章是死的", "吧唧、徽章是二次元最常见的表达物，但它只能印一张静态图，带上街之后不会回应你。"),
    ("陪伴类设备依赖手机", "现有电子宠物/电子吧唧几乎都要连 App 配网、登录、同步，脱离手机就是一块砖。"),
    ("续航与「常亮」矛盾", "想让屏幕一直显示，就得一直耗电；想省电，就得接受一块黑屏。"),
]

FEATURES = [
    ("三个角色，三种性格", "苔苔、霜霜、灯灯三套内容，头像、配色、语气、装饰各不相同，左右滑动切换。"),
    ("会看时间", "四时段问候（清晨/午后/黄昏/深夜），日期与农历、纪念日倒数。"),
    ("会听你说话", "内置中文输入，可以留一句话在徽章上；留言持久化保存。"),
    ("会数日子", "纪念日管理：新增、编辑、删除，自动算天数与倒数。"),
    ("点一下有反应", "点击互动，角色给出气泡回应，配合有限次数的真实动画。"),
    ("摇一摇有反应", "内置 IMU 检测摇动，作为「主动」交互入口（P1 新增）。"),
    ("不用时会自己睡", "15 秒变暗、30 秒真实断电熄屏，触摸唤醒（P0 新增）。"),
    ("记得时间", "RTC 双向同步，冷启动无需重新校准（P1 新增）。"),
]

TECH = [
    ("运行平台", "黄山派（SiFli SF32LB52），openvela / NuttX 实时内核 + LVGL 9.1.0"),
    ("屏幕", "390×450 AMOLED，LVGL 坐标系与物理像素一致（scale=1000）"),
    ("界面架构", "单一 UI 上下文持有全部 LVGL 对象；IMU 等外部输入经**有界事件队列**进入 controller，不直接绘图"),
    ("交互框架", "归一化事件（点击/滑动/摇动）驱动；角色切换提升 generation，旧动画回调自动过期失效"),
    ("资源策略", "字体、立绘、动画帧全部内嵌 flash；不整屏解码大量帧"),
    ("代码规模", "自研 C 源码 23 个 .c 文件，含 5 个纯逻辑模块（可脱离硬件在主机上测）"),
]

# 纯逻辑模块是技术难度里最值得讲的一点：可测
PURE_MODULES = [
    ("ebadge_power", "空闲状态机：ACTIVE→DIMMED→OFF，零阈值表示禁用该级，无符号回绕安全"),
    ("ebadge_shake", "摇动检测：滑动基线 + 触发/释放双阈值 + 冷却，用整数开方避免 ±16g 平方溢出 32 位"),
    ("ebadge_rtc_logic", "时间决策：四象限判定（主机/板端谁更可信），带偏差容忍与年份合法性"),
    ("ebadge_calendar", "农历与日期换算"),
    ("ebadge_record", "留言与纪念日记录"),
]

HIGHLIGHTS = [
    (
        "L2 真实面板断电，而不是「黑图」",
        [
            "很多「熄屏」只是把画面刷成黑色——像素还在发光，电还在耗，AMOLED 尤其明显。",
            "本作品做的是两级：15 秒 L1（暂停动画 + 半透明遮罩，视觉已暗），30 秒 L2（对 /dev/lcd0 下发 LCDDEVIO_SETPOWER=0，面板真正断电）。",
            "唤醒时先开面板再重绘，且「唤醒用的那一次触摸被消耗掉」——不会在点亮屏幕的同时误触发底下的按钮。",
            "两级的阈值可配置，为 0 即禁用该级；状态机是纯函数，主机上可完整测试。",
        ],
    ),
    (
        "摇一摇：把 IMU 做成交互入口",
        [
            "LSM6DSL 加速度计以 20 Hz 轮询（不是中断驱动，这一点如实记录）。",
            "检测算法：指数滑动基线估计重力方向，触发阈值 0.71 g、释放阈值 0.31 g，配 1.2 秒冷却防连发。",
            "加速度平方和会超出 32 位（±16 g），因此用整数开方而非浮点，避免板上无 FPU 时的性能与精度问题。",
            "开发中发现并修掉一个真实缺陷：首次触发的时间戳初值为 0，会把「从未触发」和「在第 0 毫秒触发过」混为一谈，导致开机后 1200 ms 内的摇动被冷却静默吞掉。已用显式标志位修复并加回归测试。",
        ],
    ),
    (
        "时钟持久化：徽章自己记得时间",
        [
            "没有网络，就没有 NTP。板子断电后时间会丢，每次开机都显示「未校准」，体验上很致命。",
            "方案：RTC（/dev/rtc0）与系统时间双向同步，用纯逻辑模块决策谁更可信——主机时间合法就用主机并写回 RTC；主机非法而 RTC 合法就用 RTC 回填系统时间。",
            "带 60 秒偏差容忍与 2024–2099 年份合法性校验，避免把明显错误的时间写进 RTC 覆盖掉好数据。",
            "决策逻辑完全独立于硬件访问，可在主机上穷举测试。",
        ],
    ),
]

OPENVELA_USAGE = [
    ("图形（已落地）", "LVGL 9.1 来自 openvela 的 apps/graphics/lvgl；经 CONFIG_GRAPHICS_LVGL + CONFIG_LV_USE_NUTTX / LV_USE_NUTTX_LCD / LV_USE_NUTTX_TOUCHSCREEN 接入；应用通过 openvela 自身的 Kconfig 与 manifest linkfile 注册。"),
    ("板级 BSP（已使用）", "面板、触摸、传感器与 RTC 通过 vendor BSP 暴露的设备节点访问。"),
    ("AI（未落地）", "packages/ai_agent 在 openvela manifest 中登记，但未同步进工作区；且板级无网络驱动（仅有 loopback 等），无法建立真实数据通路。**不声称已实现 AI。**"),
    ("多媒体（未落地）", "openvela vendor 树中音频仅有 CMSIS 寄存器头，没有 NuttX 音频驱动，defconfig 中 0 项音频配置。**不声称已实现多媒体。**"),
]

VERIFIED = [
    ("构建", "ninja 退出码 0，**0 warning**"),
    ("Flash 占用", "9,468,380 B / 16 MB = **56.44%**"),
    ("SRAM 占用", "107,180 B / 512 KB = **20.44%**"),
    ("纯逻辑断言", "**1622 项，0 失败**（sh tools/run_logic_test.sh）"),
    ("字形覆盖", "445 条字面量 / 282 个不同宽字符，**0 缺失**"),
    ("字体规模", "正文字形 4034 个，标题字形 153 个"),
]

NOT_RUN = [
    "屏幕电源 ioctl 是否真的让面板断电（真机未验证）",
    "触摸唤醒是否会误触发底层控件（真机未验证）",
    "加速度计读数与摇动阈值是否合适（真机未验证，阈值来自物理推导 0.71 g，不是真人手腕数据）",
    "RTC 是否能在完全断电后保住时间（真机未验证）",
    "电池独立运行时间、可佩戴外壳（未做）",
    "串口权限未取得，板子从未烧录——**所有硬件行为均为 NOT_RUN**",
]

NEXT = [
    ("P0 收尾", "电池独立运行验证、可佩戴外壳、真机 50 项验收清单"),
    ("P1 待补", "RGB/震动、系统级低功耗、电量显示、设置页"),
    ("AI 通路", "需要板级网络驱动才能真正落地；在打通之前不引入 mock 冒充"),
    ("工程", "把 nuttx/ 工作区残缺问题在干净环境中复现并修正"),
]

# ---------------------------------------------------------------- PDF

def build_pdf(path: Path) -> None:
    from reportlab.lib.pagesizes import A4
    from reportlab.lib.units import mm
    from reportlab.lib import colors
    from reportlab.lib.styles import ParagraphStyle
    from reportlab.platypus import (
        BaseDocTemplate, Frame, PageTemplate, Paragraph, Spacer,
        Table, TableStyle, KeepTogether,
    )
    from reportlab.pdfbase import pdfmetrics
    from reportlab.pdfbase.ttfonts import TTFont

    pdfmetrics.registerFont(TTFont("CJK", ensure_pdf_font()))
    # 注册字体族，使 <b> 粗体标签仍用同一内嵌字体，而不是回退到未嵌入的 Helvetica
    pdfmetrics.registerFontFamily("CJK", normal="CJK", bold="CJK",
                                  italic="CJK", boldItalic="CJK")

    ACCENT = colors.HexColor("#FF6B9D")
    DARK = colors.HexColor("#2B2B3A")
    GREY = colors.HexColor("#6A6A7A")
    LIGHT = colors.HexColor("#FDF0F5")

    def S(name, **kw):
        base = dict(fontName="CJK", fontSize=10, leading=16, textColor=DARK)
        base.update(kw)
        return ParagraphStyle(name, **base)

    st_h1 = S("h1", fontSize=19, leading=26, textColor=ACCENT, spaceAfter=6)
    st_h2 = S("h2", fontSize=13.5, leading=20, textColor=DARK, spaceBefore=13, spaceAfter=5)
    st_h3 = S("h3", fontSize=11.5, leading=17, textColor=ACCENT, spaceBefore=9, spaceAfter=3)
    st_p = S("p")
    st_li = S("li", leftIndent=11, bulletIndent=2)
    st_meta = S("meta", fontSize=9, textColor=GREY)
    st_note = S("note", fontSize=9, leading=14, textColor=GREY)

    def h1(t): return Paragraph(em_pdf(t), st_h1)
    def h2(t): return Paragraph(em_pdf(t), st_h2)
    def h3(t): return Paragraph(em_pdf(t), st_h3)
    def p(t): return Paragraph(em_pdf(t), st_p)
    def li(t): return Paragraph(em_pdf(t), st_li, bulletText="•")
    def note(t): return Paragraph(em_pdf(t), st_note)
    def sp(h=6): return Spacer(1, h)

    def band(title, rows, c0=34 * mm):
        data = [[Paragraph(em_pdf(f"<b>{a}</b>"), st_p), Paragraph(em_pdf(b), st_p)] for a, b in rows]
        t = Table(data, colWidths=[c0, None])
        t.setStyle(TableStyle([
            ("VALIGN", (0, 0), (-1, -1), "TOP"),
            ("TOPPADDING", (0, 0), (-1, -1), 4),
            ("BOTTOMPADDING", (0, 0), (-1, -1), 4),
            ("LEFTPADDING", (0, 0), (-1, -1), 6),
            ("RIGHTPADDING", (0, 0), (-1, -1), 6),
            ("BACKGROUND", (0, 0), (0, -1), LIGHT),
            ("LINEBELOW", (0, 0), (-1, -2), 0.3, colors.HexColor("#E8E8EF")),
            ("BOX", (0, 0), (-1, -1), 0.4, colors.HexColor("#E0E0EA")),
        ]))
        return t

    story = []
    story += [h1(TITLE), note(f"{SUBTITLE}　|　赛道：{TRACK}"), sp(4)]
    story += [p(ONELINER), sp(4)]
    story += [band("作品信息", [
        ("作品名称", TITLE),
        ("所属赛道", TRACK),
        ("运行平台", "openvela / NuttX + LVGL 9.1，黄山派 SF32LB52，390×450 AMOLED"),
        ("团队仓", "open-vela/contest2026_462_shijichuangyou"),
        ("开源许可", "Apache License 2.0"),
    ])]

    story += [h2("一、要解决的问题")]
    for a, b in PAIN:
        story += [h3(a), p(b)]

    story += [h2("二、核心功能")]
    for a, b in FEATURES:
        story += [li(f"<b>{a}</b>：{b}")]

    story += [h2("三、技术实现")]
    story += [band("平台与架构", TECH)]

    story += [h3("纯逻辑模块（可脱离硬件在主机上完整测试）")]
    story += [p("这是本作品工程质量上最值得说明的一点：把「什么时候熄屏」「摇动算不算触发」「该信主机还是信 RTC」这类判断，全部写成不依赖 LVGL 与硬件的纯函数，因此可以在主机上穷举测试，而不是只能靠真机撞。")]
    for a, b in PURE_MODULES:
        story += [li(f"<b>{a}</b>：{b}")]

    story += [h2("四、三个技术亮点")]
    for i, (t, pts) in enumerate(HIGHLIGHTS, 1):
        block = [h3(f"{i}. {t}")] + [li(x) for x in pts]
        story += [KeepTogether(block)]

    story += [h2("五、openvela 能力使用说明")]
    story += [p("《大赛总览》要求：项目须使用 openvela 开源项目（<b>NuttX 内核仓库除外</b>）提供的系统能力，且至少落地图形、AI、多媒体三项核心能力之一。")]
    story += [band("逐条对照", OPENVELA_USAGE, c0=30 * mm)]
    story += [note("说明：图形一项已落地并有代码与配置证据。AI 与多媒体<b>未落地</b>，原因如上——不以此充当已完成的能力。")]

    story += [h2("六、验证状态（诚实声明）")]
    story += [h3("已在主机侧实测通过")]
    story += [band("实测数据", VERIFIED, c0=30 * mm)]
    story += [h3("从未验证（真机全部 NOT_RUN）")]
    story += [p("串口权限未取得、板子未烧录，因此以下全部<b>没有</b>真机结论：")]
    for x in NOT_RUN:
        story += [li(x)]
    story += [note("本材料不提供续航时间数字，也不声称「熄屏等于深睡」。「编译通过 ≠ 真机通过」。")]

    story += [h2("七、如何运行")]
    story += [band("步骤", [
        ("1. 获取源码", "repo init -u <openvela manifest> -b dev-ai-contest-2026 && repo sync"),
        ("2. 配置", "启用 CONFIG_LVX_USE_DEMO_CONTEST2026_462_EBADGE=y；确认 CONFIG_GRAPHICS_LVGL=y、CONFIG_LV_USE_NUTTX=y"),
        ("3. 构建", "cmake -G Ninja ... && ninja（详见仓根 README §4.2）"),
        ("4. 烧录", "sftool -c SF32LB52 -p <串口> write_flash nuttx.bin@0x12010000"),
        ("5. 运行", "上电自动启动 ebadge 应用；左右滑动切角色、点击互动、摇动触发反应"),
    ], c0=26 * mm)]
    story += [note("※ 本工作区 nuttx/ 有 23986 个受跟踪文件处于已暂存删除状态，构建实际使用 nuttx-ebadge-clean/。干净 repo sync 出的新工作区应使用 nuttx/。不要在 nuttx/ 内执行 git commit。详见 README §4.2。")]

    story += [h2("八、后续规划")]
    story += [band("计划", NEXT, c0=26 * mm)]

    doc = BaseDocTemplate(
        str(path), pagesize=A4,
        leftMargin=20 * mm, rightMargin=20 * mm,
        topMargin=18 * mm, bottomMargin=18 * mm,
        title=TITLE, author="mumu-bird",
    )
    frame = Frame(doc.leftMargin, doc.bottomMargin, doc.width, doc.height, id="f")

    def on_page(canv, d):
        canv.saveState()
        canv.setFont("CJK", 8)
        canv.setFillColor(GREY)
        canv.drawString(20 * mm, 11 * mm, f"{TITLE}　·　{TRACK}")
        canv.drawRightString(A4[0] - 20 * mm, 11 * mm, f"第 {canv.getPageNumber()} 页")
        canv.setStrokeColor(colors.HexColor("#F0D8E4"))
        canv.line(20 * mm, 14 * mm, A4[0] - 20 * mm, 14 * mm)
        canv.restoreState()

    doc.addPageTemplates([PageTemplate(id="all", frames=[frame], onPage=on_page)])
    doc.build(story)


# ---------------------------------------------------------------- DOCX

def build_docx(path: Path) -> None:
    from docx import Document
    from docx.shared import Pt, RGBColor, Cm
    from docx.oxml.ns import qn
    from docx.enum.text import WD_ALIGN_PARAGRAPH

    doc = Document()
    st = doc.styles["Normal"]
    st.font.name = CJK_LATIN
    st.font.size = Pt(10.5)
    st.element.rPr.rFonts.set(qn("w:eastAsia"), CJK_NAME)

    for s in doc.sections:
        s.left_margin = s.right_margin = Cm(2.2)
        s.top_margin = s.bottom_margin = Cm(2.0)

    ACCENT = RGBColor(0xFF, 0x6B, 0x9D)
    DARK = RGBColor(0x2B, 0x2B, 0x3A)

    def para(text="", size=10.5, bold=False, color=None, indent=0, bullet=False, space_after=4):
        q = doc.add_paragraph()
        if bullet:
            q.style = doc.styles["List Bullet"]
        if indent:
            q.paragraph_format.left_indent = Cm(indent)
        q.paragraph_format.space_after = Pt(space_after)
        prefix = "• " if (bullet and q.style.name != "List Bullet") else ""
        # 处理 **强调**：拆成多个 run，真实加粗+主题色（而不是把星号原样打出来）
        for part in re.split(r"(\*\*.+?\*\*)", prefix + text):
            if not part:
                continue
            is_em = part.startswith("**") and part.endswith("**") and len(part) >= 4
            r = q.add_run(part[2:-2] if is_em else part)
            r.font.size = Pt(size)
            r.bold = bold or is_em
            r.font.name = CJK_LATIN
            r._element.rPr.rFonts.set(qn("w:eastAsia"), CJK_NAME)
            if color or is_em:
                r.font.color.rgb = color or ACCENT
        return q

    def head(text, size=13.5, color=None, before=10):
        q = doc.add_paragraph()
        q.paragraph_format.space_before = Pt(before)
        q.paragraph_format.space_after = Pt(4)
        r = q.add_run(em_plain(text))
        r.font.size = Pt(size)
        r.bold = True
        r.font.name = CJK_LATIN
        r._element.rPr.rFonts.set(qn("w:eastAsia"), CJK_NAME)
        r.font.color.rgb = color or DARK
        return q

    def table(rows, w0=3.2):
        t = doc.add_table(rows=0, cols=2)
        t.style = "Light Grid Accent 1"
        for a, b in rows:
            c = t.add_row().cells
            c[0].width = Cm(w0)
            for cell, txt, bold in ((c[0], a, True), (c[1], b, False)):
                cell.text = ""
                pr = cell.paragraphs[0]
                pr.paragraph_format.space_after = Pt(1)
                rr = pr.add_run(em_plain(txt))
                rr.font.size = Pt(9.5)
                rr.bold = bold
                rr.font.name = CJK_LATIN
                rr._element.rPr.rFonts.set(qn("w:eastAsia"), CJK_NAME)
        return t

    ti = doc.add_paragraph()
    ti.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = ti.add_run(TITLE)
    r.font.size = Pt(20); r.bold = True; r.font.color.rgb = ACCENT
    r.font.name = CJK_LATIN
    r._element.rPr.rFonts.set(qn("w:eastAsia"), CJK_NAME)

    sub = doc.add_paragraph()
    sub.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r = sub.add_run(f"{SUBTITLE}　|　赛道：{TRACK}")
    r.font.size = Pt(10.5); r.font.color.rgb = RGBColor(0x6A, 0x6A, 0x7A)
    r.font.name = CJK_LATIN
    r._element.rPr.rFonts.set(qn("w:eastAsia"), CJK_NAME)

    para(ONELINER, space_after=8)
    table([
        ("作品名称", TITLE),
        ("所属赛道", TRACK),
        ("运行平台", "openvela / NuttX + LVGL 9.1，黄山派 SF32LB52，390×450 AMOLED"),
        ("团队仓", "open-vela/contest2026_462_shijichuangyou"),
        ("开源许可", "Apache License 2.0"),
    ])

    head("一、要解决的问题", 13.5, ACCENT)
    for a, b in PAIN:
        para(a, bold=True, size=11)
        para(b)

    head("二、核心功能", 13.5, ACCENT)
    for a, b in FEATURES:
        para(f"{a}：{b}", bullet=True)

    head("三、技术实现", 13.5, ACCENT)
    table(TECH, w0=3.0)
    para("纯逻辑模块（可脱离硬件在主机上完整测试）", bold=True, size=11)
    para("这是工程质量上最值得说明的一点：把「什么时候熄屏」「摇动算不算触发」「该信主机还是信 RTC」这类判断，全部写成不依赖 LVGL 与硬件的纯函数，因此可以在主机上穷举测试。")
    for a, b in PURE_MODULES:
        para(f"{a}：{b}", bullet=True)

    head("四、三个技术亮点", 13.5, ACCENT)
    for i, (t, pts) in enumerate(HIGHLIGHTS, 1):
        para(f"{i}. {t}", bold=True, size=11)
        for x in pts:
            para(x, bullet=True)

    head("五、openvela 能力使用说明", 13.5, ACCENT)
    para("《大赛总览》要求：项目须使用 openvela 开源项目（NuttX 内核仓库除外）提供的系统能力，且至少落地图形、AI、多媒体三项核心能力之一。")
    table(OPENVELA_USAGE, w0=3.0)
    para("图形一项已落地并有代码与配置证据。AI 与多媒体未落地，原因如上——不以此充当已完成的能力。", size=9.5)

    head("六、验证状态（诚实声明）", 13.5, ACCENT)
    para("已在主机侧实测通过", bold=True, size=11)
    table(VERIFIED, w0=3.0)
    para("从未验证（真机全部 NOT_RUN）", bold=True, size=11)
    para("串口权限未取得、板子未烧录，因此以下全部没有真机结论：")
    for x in NOT_RUN:
        para(x, bullet=True)
    para("本材料不提供续航时间数字，也不声称「熄屏等于深睡」。「编译通过 ≠ 真机通过」。", size=9.5)

    head("七、如何运行", 13.5, ACCENT)
    table([
        ("1. 获取源码", "repo init -u <openvela manifest> -b dev-ai-contest-2026 && repo sync"),
        ("2. 配置", "启用 CONFIG_LVX_USE_DEMO_CONTEST2026_462_EBADGE=y；确认 CONFIG_GRAPHICS_LVGL=y、CONFIG_LV_USE_NUTTX=y"),
        ("3. 构建", "cmake -G Ninja ... && ninja（详见仓根 README §4.2）"),
        ("4. 烧录", "sftool -c SF32LB52 -p <串口> write_flash nuttx.bin@0x12010000"),
        ("5. 运行", "上电自动启动 ebadge 应用；左右滑动切角色、点击互动、摇动触发反应"),
    ], w0=2.8)
    para("※ 本工作区 nuttx/ 有 23986 个受跟踪文件处于已暂存删除状态，构建实际使用 nuttx-ebadge-clean/；干净 repo sync 出的新工作区应使用 nuttx/。不要在 nuttx/ 内执行 git commit。", size=9.5)

    head("八、后续规划", 13.5, ACCENT)
    table(NEXT, w0=2.8)

    doc.save(str(path))


# ---------------------------------------------------------------- PPTX

def build_pptx(path: Path) -> None:
    from pptx import Presentation
    from pptx.util import Inches, Pt, Emu
    from pptx.dml.color import RGBColor
    from pptx.enum.text import PP_ALIGN, MSO_ANCHOR
    from pptx.oxml.ns import qn

    PINK = RGBColor(0xFF, 0x6B, 0x9D)
    DARK = RGBColor(0x2B, 0x2B, 0x3A)
    GREY = RGBColor(0x6A, 0x6A, 0x7A)
    WHITE = RGBColor(0xFF, 0xFF, 0xFF)
    SOFT = RGBColor(0xFD, 0xF0, 0xF5)

    prs = Presentation()
    prs.slide_width = Inches(13.333)
    prs.slide_height = Inches(7.5)
    W, H = prs.slide_width, prs.slide_height
    blank = prs.slide_layouts[6]

    def cjk(run):
        run.font.name = CJK_LATIN
        rPr = run._r.get_or_add_rPr()
        ea = rPr.find(qn("a:ea"))
        if ea is None:
            ea = rPr.makeelement(qn("a:ea"), {})
            rPr.append(ea)
        ea.set("typeface", CJK_NAME)

    def bg(slide, color):
        f = slide.background.fill
        f.solid()
        f.fore_color.rgb = color

    def rect(slide, x, y, w, h, color, line=None):
        from pptx.enum.shapes import MSO_SHAPE
        s = slide.shapes.add_shape(MSO_SHAPE.ROUNDED_RECTANGLE, x, y, w, h)
        s.fill.solid(); s.fill.fore_color.rgb = color
        if line:
            s.line.color.rgb = line; s.line.width = Pt(1)
        else:
            s.line.fill.background()
        s.shadow.inherit = False
        return s

    def tb(slide, x, y, w, h, text, size=18, bold=False, color=DARK, align=PP_ALIGN.LEFT,
           anchor=MSO_ANCHOR.TOP, spacing=1.15):
        box = slide.shapes.add_textbox(x, y, w, h)
        tf = box.text_frame
        tf.word_wrap = True
        tf.vertical_anchor = anchor
        lines = text.split("\n")
        for i, ln in enumerate(lines):
            para = tf.paragraphs[0] if i == 0 else tf.add_paragraph()
            para.alignment = align
            para.line_spacing = spacing
            r = para.add_run(); r.text = em_plain(ln)
            r.font.size = Pt(size); r.bold = bold; r.font.color.rgb = color
            cjk(r)
        return box

    def bullets(slide, x, y, w, h, items, size=15, color=DARK, gap=9):
        # 允许传入 (head, body) 元组或纯字符串两种形式
        norm = []
        for it in items:
            if isinstance(it, (tuple, list)):
                h_, b_ = it[0], (it[1] if len(it) > 1 else "")
            elif "：" in it:
                h_, b_ = it.split("：", 1)
            else:
                h_, b_ = it, ""
            norm.append((h_, b_))
        items = norm
        box = slide.shapes.add_textbox(x, y, w, h)
        tf = box.text_frame; tf.word_wrap = True
        for i, (head, body) in enumerate(items):
            para = tf.paragraphs[0] if i == 0 else tf.add_paragraph()
            para.space_after = Pt(gap); para.line_spacing = 1.2
            r = para.add_run(); r.text = f"● {em_plain(head)}"
            r.font.size = Pt(size); r.bold = True; r.font.color.rgb = PINK; cjk(r)
            if body:
                para2 = tf.add_paragraph(); para2.space_after = Pt(gap)
                para2.line_spacing = 1.25
                r2 = para2.add_run(); r2.text = f"   {em_plain(body)}"
                r2.font.size = Pt(size - 1.5); r2.font.color.rgb = color; cjk(r2)
        return box

    def pagenum(slide, n, total=12):
        tb(slide, W - Inches(1.3), H - Inches(0.62), Inches(1.0), Inches(0.4),
           f"{n} / {total}", size=11, color=GREY, align=PP_ALIGN.RIGHT)

    def title_bar(slide, text, sub=None, n=None):
        bg(slide, WHITE)
        rect(slide, 0, 0, Inches(0.16), H, PINK)
        tb(slide, Inches(0.75), Inches(0.5), W - Inches(1.5), Inches(0.8),
           text, size=30, bold=True, color=DARK)
        if sub:
            tb(slide, Inches(0.78), Inches(1.28), W - Inches(1.6), Inches(0.5),
               sub, size=14, color=GREY)
        if n:
            pagenum(slide, n)

    # 1 封面
    s = prs.slides.add_slide(blank); bg(s, PINK)
    tb(s, Inches(1.0), Inches(2.0), W - Inches(2), Inches(1.4), TITLE,
       size=52, bold=True, color=WHITE, align=PP_ALIGN.CENTER)
    tb(s, Inches(1.0), Inches(3.4), W - Inches(2), Inches(0.6), SUBTITLE,
       size=22, color=WHITE, align=PP_ALIGN.CENTER)
    tb(s, Inches(1.0), Inches(4.3), W - Inches(2), Inches(0.5),
       f"赛道：{TRACK}", size=16, color=WHITE, align=PP_ALIGN.CENTER)
    tb(s, Inches(1.0), Inches(6.3), W - Inches(2), Inches(0.4),
       "openvela / NuttX + LVGL 9.1　·　黄山派 SF32LB52　·　390×450 AMOLED",
       size=13, color=WHITE, align=PP_ALIGN.CENTER)

    # 2 痛点
    s = prs.slides.add_slide(blank)
    title_bar(s, "徽章是死的", "二次元最常见的表达物，却不会回应你", 2)
    bullets(s, Inches(0.9), Inches(2.0), W - Inches(1.8), Inches(4.4), [
        (a, b) for a, b in PAIN
    ], size=17, gap=14)

    # 3 产品形态
    s = prs.slides.add_slide(blank)
    title_bar(s, "一块会回应的吧唧", "戴在身上的陪伴物", 3)
    tb(s, Inches(0.9), Inches(2.0), W - Inches(1.8), Inches(1.6), ONELINER,
       size=17, spacing=1.4)
    bullets(s, Inches(0.9), Inches(4.0), W - Inches(1.8), Inches(2.4), [
        ("开机即用", "不需要配网、登录、连手机"),
        ("全中文界面", "面向中文二次元用户"),
        ("离线可用", "所有内容内嵌在设备里"),
    ], size=15)

    # 4 核心功能
    s = prs.slides.add_slide(blank)
    title_bar(s, "核心功能", "三个角色 · 八项能力", 4)
    left = FEATURES[:4]; right = FEATURES[4:]
    bullets(s, Inches(0.8), Inches(1.95), Inches(5.8), Inches(4.8),
            [a + "：" + b for a, b in left], size=13.5, gap=10)
    bullets(s, Inches(6.9), Inches(1.95), Inches(5.8), Inches(4.8),
            [a + "：" + b for a, b in right], size=13.5, gap=10)

    # 5 技术架构
    s = prs.slides.add_slide(blank)
    title_bar(s, "技术架构", "单 UI 上下文 · 事件队列 · 纯逻辑可测", 5)
    bullets(s, Inches(0.9), Inches(1.95), W - Inches(1.8), Inches(3.0), [
        ("单一 UI 上下文", "全部 LVGL 对象由一个上下文持有；IMU 等外部输入经有界事件队列进入 controller，不直接绘图"),
        ("generation 过期机制", "角色切换提升 generation，旧动画与旧回调自动失效"),
        ("资源内嵌", "字体 / 立绘 / 动画帧全部编译进 flash，不整屏解码"),
    ], size=15, gap=11)
    rect(s, Inches(0.9), Inches(5.5), W - Inches(1.8), Inches(1.25), SOFT)
    tb(s, Inches(1.2), Inches(5.62), W - Inches(2.4), Inches(1.0),
       "关键取舍：把「何时熄屏」「摇动是否触发」「该信主机还是信 RTC」写成不依赖硬件的纯函数\n→ 可在主机上穷举测试，而不是只能靠真机撞", size=14, color=DARK, spacing=1.35)

    # 6-8 三个亮点
    for idx, (t, pts) in enumerate(HIGHLIGHTS, 6):
        s = prs.slides.add_slide(blank)
        title_bar(s, t, None, idx)
        bullets(s, Inches(0.9), Inches(1.8), W - Inches(1.8), Inches(5.0),
                [(x.split("：")[0], "：".join(x.split("：")[1:]) if "：" in x else "") for x in pts],
                size=14, gap=10)

    # 9 openvela 能力
    s = prs.slides.add_slide(blank)
    title_bar(s, "openvela 能力使用", "硬性资格：图形 / AI / 多媒体 至少一项", 9)
    tb(s, Inches(0.9), Inches(1.8), W - Inches(1.8), Inches(0.6),
       "图形已落地：LVGL 9.1 来自 openvela apps 树，经 CONFIG_GRAPHICS_LVGL 接入，应用经官方 Kconfig 与 manifest 注册",
       size=14, color=DARK, spacing=1.3)
    bullets(s, Inches(0.9), Inches(2.85), W - Inches(1.8), Inches(3.8), [
        ("图形 —— 已落地", "LVGL 9.1（apps/graphics/lvgl）+ NuttX LCD/触摸后端"),
        ("板级 BSP —— 已使用", "面板、触摸、传感器、RTC 设备节点"),
        ("AI —— 未落地", "ai_agent 已登记但未同步，且板级无网络驱动；不声称已实现"),
        ("多媒体 —— 未落地", "vendor 树音频仅 CMSIS 寄存器头，无 NuttX 音频驱动；不声称已实现"),
    ], size=14, gap=10)

    # 10 验证
    s = prs.slides.add_slide(blank)
    title_bar(s, "验证状态", "主机侧实测通过，真机全部 NOT_RUN", 10)
    left = VERIFIED[:3]; right = VERIFIED[3:]
    bullets(s, Inches(0.8), Inches(1.95), Inches(5.8), Inches(2.6),
            [a + "：" + b.replace("**", "") for a, b in left], size=14, gap=10)
    bullets(s, Inches(6.9), Inches(1.95), Inches(5.8), Inches(2.6),
            [a + "：" + b.replace("**", "") for a, b in right], size=14, gap=10)
    rect(s, Inches(0.8), Inches(4.85), W - Inches(1.6), Inches(1.85), SOFT)
    tb(s, Inches(1.1), Inches(4.98), W - Inches(2.2), Inches(1.6),
       "未验证：屏幕电源 ioctl · 触摸唤醒 · 加速度计读数 · RTC 掉电保时 · 电池续航 · 可佩戴外壳\n"
       "串口权限未取得，板子从未烧录 —— 不提供续航数字，不声称「熄屏 = 深睡」，编译通过 ≠ 真机通过",
       size=13, color=DARK, spacing=1.35)

    # 11 边界与规划
    s = prs.slides.add_slide(blank)
    title_bar(s, "诚实的边界与下一步", "知道没做什么，比声称都做了重要", 11)
    bullets(s, Inches(0.9), Inches(1.95), W - Inches(1.8), Inches(4.5), [
        ("已完成（P0 + 部分 P1）", "角色切换 / 点击互动 / 真实动画 / 自动熄屏 / 留言 / 纪念日 / IMU 摇动 / RTC 持久化"),
        ("未做（P1 剩余）", "RGB 与震动反馈、系统级低功耗、电量显示、设置页"),
        ("未做（P0 剩余）", "电池独立运行实测、可佩戴外壳、真机 50 项验收"),
        ("AI 通路", "需要板级网络驱动才能真正落地；在打通前不引入 mock 冒充真实结果"),
    ], size=14, gap=12)

    # 12 结语
    s = prs.slides.add_slide(blank); bg(s, PINK)
    tb(s, Inches(1.0), Inches(2.5), W - Inches(2), Inches(1.4),
       "让吧唧，不再是一张死图", size=40, bold=True, color=WHITE, align=PP_ALIGN.CENTER)
    tb(s, Inches(1.0), Inches(4.1), W - Inches(2), Inches(0.6),
       TITLE, size=20, color=WHITE, align=PP_ALIGN.CENTER)
    tb(s, Inches(1.0), Inches(5.6), W - Inches(2), Inches(0.9),
       "代码遵循 Apache License 2.0\nopen-vela/contest2026_462_shijichuangyou",
       size=13, color=WHITE, align=PP_ALIGN.CENTER, spacing=1.4)

    prs.save(str(path))


# ---------------------------------------------------------------- 视频脚本

def build_video_script(path: Path) -> None:
    md = f"""# 演示视频脚本 · {TITLE}

> 官方要求：**演示视频（不超过 5 分钟，mp4 / mov 等常见格式）**，与作品介绍文档、专属仓地址一并提交。
> 本脚本按 **4 分 40 秒**设计，留出安全余量。旁白可直接照读。

## 录制前须知（重要）

本脚本按"**真机已烧录**"编写。若板子尚未烧录，**不要**用电脑模拟冒充真机画面——
官方明确禁止用 mock/replay 冒充真实结果。此时可选：
1. 先完成烧录再录（推荐）；
2. 或录制时明确标注"主机构建产物 / 界面设计稿"，并在旁白中说明真机状态。

## 分镜表

| 时间 | 画面 | 旁白 | 备注 |
| --- | --- | --- | --- |
| 0:00–0:12 | 黑底白字标题淡入：拾迹创游 · 互动电子吧唧 | "这是一块会回应你的电子吧唧。" | 可配轻快 BGM |
| 0:12–0:32 | 手持徽章上电，屏幕亮起，出现角色与问候语 | "它叫拾迹创游。三位二次元角色——苔苔、霜霜、灯灯，陪着你看时间、留一句话、数纪念日。开机即用，不连手机、不需要网络。" | 重点：真实上电过程 |
| 0:32–0:58 | 左右滑动切换三个角色，展示不同配色与问候 | "左右滑动就能换角色。每个角色的头像、配色、语气都不一样，问候语还会跟着时间变——清晨、午后、黄昏、深夜各不同。" | 一镜到底最好 |
| 0:58–1:20 | 点击角色 → 气泡回应 + 动画 | "点一下，她会回应你。这是真实的动画，不是静态图换帧。" | 展示动画流畅度 |
| 1:20–1:45 | 输入一句中文留言，保存，返回查看 | "还能留一句话在徽章上。内置中文输入，写下的内容会保存下来，下次开机还在。" | 展示输入法 |
| 1:45–2:10 | 纪念日页面：新增、倒数天数 | "纪念日可以自己加，它会帮你数着日子。" | |
| 2:10–2:50 | **重点**：计时到 15 秒画面变暗 → 30 秒屏幕完全黑掉；用万用表/功耗计或特写说明面板断电 | "很多所谓的熄屏，只是把画面刷成黑色——像素还在发光，电还在耗。我们做的是两级：15 秒变暗，30 秒屏幕真正断电。" | **本节最关键**，尽量给出可判断的证据 |
| 2:50–3:10 | 触摸屏幕 → 立刻亮起；再点一次按钮才响应 | "碰一下就能唤醒。而且唤醒用的那一次触摸会被消耗掉——不会在点亮屏幕的同时误按到底下的按钮。" | 体现细节打磨 |
| 3:10–3:40 | 摇动徽章 → 角色做出反应 | "摇一摇，她也会有反应。里面是一颗加速度计，20 赫兹采样，靠滑动基线判断重力方向，触发阈值 0.71 g。" | 手上动作要明显 |
| 3:40–4:10 | 断电 → 重新上电 → 时间正确显示 | "没有网络就没有对时。板子断电后时间本来会丢，我们让它跟 RTC 双向同步——现在重新上电，时间还是准的。" | **第二关键**：体现断电重来 |
| 4:10–4:35 | 板子特写 + 屏幕上显示 openvela/LVGL 信息 | "整个应用跑在 openvela 上，界面用 LVGL 9.1。图形能力来自 openvela 的 apps 仓库，应用通过官方 Kconfig 和 manifest 注册接入。" | |
| 4:35–4:40 | 结尾：作品名 + 仓库地址 | "代码全部开源，遵循 Apache 2.0。谢谢观看。" | 定格 |

## 旁白全文（连续朗读版，约 4 分 20 秒）

> 这是一块会回应你的电子吧唧。
>
> 它叫拾迹创游。三位二次元角色——苔苔、霜霜、灯灯，陪着你看时间、留一句话、数纪念日。开机即用，不连手机、不需要网络。
>
> 左右滑动就能换角色。每个角色的头像、配色、语气都不一样，问候语还会跟着时间变——清晨、午后、黄昏、深夜各不同。
>
> 点一下，她会回应你。这是真实的动画，不是静态图换帧。
>
> 还能留一句话在徽章上。内置中文输入，写下的内容会保存下来，下次开机还在。纪念日也可以自己加，它会帮你数着日子。
>
> 接下来是我最想讲的部分。很多所谓的熄屏，只是把画面刷成黑色——像素还在发光，电还在耗。我们做的是两级：十五秒画面变暗，三十秒屏幕真正断电。
>
> 碰一下就能唤醒。而且唤醒用的那一次触摸会被消耗掉——不会在点亮屏幕的同时误按到底下的按钮。
>
> 摇一摇，她也会有反应。里面是一颗加速度计，二十赫兹采样，靠滑动基线判断重力方向，触发阈值零点七一克。
>
> 没有网络就没有对时。板子断电后时间本来会丢，我们让它跟 RTC 双向同步——现在重新上电，时间还是准的。
>
> 整个应用跑在 openvela 上，界面用 LVGL 九点一。图形能力来自 openvela 的 apps 仓库，应用通过官方 Kconfig 和 manifest 注册接入。
>
> 代码全部开源，遵循 Apache 二点零。谢谢观看。

## 拍摄清单

- [ ] 板子已烧录并能正常启动
- [ ] 电量充足（避免录制中断电）
- [ ] 背景干净，光线充足（屏幕内容要拍得清）
- [ ] 手机/相机固定，避免抖动
- [ ] 特写镜头：拍清屏幕 UI 细节
- [ ] 若展示"面板断电"，尽量用功耗计或明确的可判断证据
- [ ] 录制隔音，或后期配旁白
- [ ] 输出 mp4，控制在 5 分钟内

## 诚实性约束（录制时务必遵守）

1. **不伪造真机画面**：未烧录就不要用模拟画面冒充。
2. **不夸大熄屏**：不要暗示"深睡"或给出未测的续航数字。
3. **不谎称 AI**：本作品没有落地 AI 能力，视频中不要暗示有语音/AI 交互。
4. **未验证的点不要演示**：如果某个功能在真机上没通过，就不要放进视频。
"""
    path.write_text(md, encoding="utf-8")


def main() -> int:
    build_pdf(OUT / "拾迹创游-作品介绍.pdf")
    print("OK  submission/拾迹创游-作品介绍.pdf")
    build_docx(OUT / "拾迹创游-作品介绍.docx")
    print("OK  submission/拾迹创游-作品介绍.docx")
    build_pptx(OUT / "拾迹创游-路演PPT.pptx")
    print("OK  submission/拾迹创游-路演PPT.pptx")
    build_video_script(OUT / "拾迹创游-演示视频脚本.md")
    print("OK  submission/拾迹创游-演示视频脚本.md")
    return 0


if __name__ == "__main__":
    sys.exit(main())
