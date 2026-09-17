#!/usr/bin/env python3
"""Offline THA3 experiment. Never uploads portraits or changes firmware."""

import argparse
import hashlib
import json
import sys
import time
from datetime import datetime, timezone
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT.parents[1] / ".tools/tha3-motion"
MODULES = (
    "eyebrow_decomposer", "eyebrow_morphing_combiner", "face_morpher",
    "two_algo_face_body_rotator", "editor",
)
POSES = {
    "neutral": {},
    "blink035": {"eye_wink_left": 0.35, "eye_wink_right": 0.35},
    "blink070": {"eye_wink_left": 0.70, "eye_wink_right": 0.70},
    "blink100": {"eye_wink_left": 1.0, "eye_wink_right": 1.0},
    "right004": {"head_y": 0.04},
    "right008": {"head_y": 0.08},
    "right012": {"head_y": 0.12},
    "left004": {"head_y": -0.04},
    "left008": {"head_y": -0.08},
    "left012": {"head_y": -0.12},
}
CLIPS = {
    "blink": (["neutral", "blink035", "blink070", "blink100",
               "blink070", "blink035", "neutral"],
              [1800, 60, 60, 100, 60, 60, 1200]),
    "head-turn": (["neutral", "right004", "right008", "right012",
                   "right008", "right004", "neutral", "left004",
                   "left008", "left012", "left008", "left004", "neutral"],
                  [1400, 120, 120, 400, 120, 120, 500, 120,
                   120, 400, 120, 120, 1400]),
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def screen(frame, title="苔苔", caption="把小小的快乐，藏进口袋"):
    canvas = Image.new("RGBA", (390, 450), "#f6f3e7")
    portrait = frame.crop((112, 0, 400, 288)).resize((390, 390), Image.Resampling.LANCZOS)
    canvas.alpha_composite(portrait, (0, 0))
    draw = ImageDraw.Draw(canvas)
    title_font = ImageFont.truetype("/usr/share/fonts/opentype/noto/NotoSerifCJK-Regular.ttc", 23)
    caption_font = ImageFont.truetype("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", 15)
    draw.text((195, 386), title, font=title_font, fill="#354d32", anchor="mt")
    draw.text((195, 421), caption, font=caption_font, fill="#627059", anchor="mt")
    return canvas.convert("RGB")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", choices=("prepare", "keyframes", "clips"), default="keyframes")
    parser.add_argument("--source", type=Path, default=ROOT / "assets/portraits/taitai-v1.png")
    parser.add_argument("--upstream", type=Path, default=BASE / "talking-head-anime-3-demo-main")
    parser.add_argument("--models", type=Path, default=BASE / "models/separable_float")
    parser.add_argument("--output", type=Path, default=ROOT / "design/taitai-motion-tha3")
    parser.add_argument("--threads", type=int, default=4)
    args = parser.parse_args()
    out = args.output
    out.mkdir(parents=True, exist_ok=True)
    (out / "frames").mkdir(exist_ok=True)
    original = Image.open(args.source).convert("RGBA")
    fitted = original.copy()
    fitted.thumbnail((256, 256), Image.Resampling.LANCZOS)
    position = ((512 - fitted.width) // 2, 8)
    source = Image.new("RGBA", (512, 512))
    source.alpha_composite(fitted, position)
    source.save(out / "input-512.png")
    guide = Image.new("RGBA", (512, 512), "#eceadf")
    guide.alpha_composite(source)
    pen = ImageDraw.Draw(guide)
    pen.rectangle((192, 64, 320, 192), outline="#be503c", width=2)
    pen.rectangle((160, 32, 352, 224), outline="#4b825a", width=1)
    guide.convert("RGB").save(out / "input-guide.png")
    screen(source, caption="原图输入，未经神经网络处理").save(out / "original-screen.png")
    record = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "stage": args.stage, "source": str(args.source),
        "source_sha256": digest(args.source),
        "engine": "Talking Head Anime 3 / separable_float / CPU",
        "upstream": "https://github.com/pkhungurn/talking-head-anime-3-demo",
        "code_license": "MIT", "model_license": "CC-BY-4.0",
        "model_author": "pkhungurn", "firmware_changed": False,
        "normalization": {"canvas": [512, 512], "portrait": list(fitted.size),
                          "position": list(position), "screen": [390, 450],
                          "preview_crop": [112, 0, 400, 288]},
        "pose_baseline": "All 45 parameters zero, including mouth_aaa (closed mouth).",
        "frames": [], "quality_status": "Experimental; visual approval required.",
    }

    def save_record():
        (out / "generation.json").write_text(json.dumps(record, ensure_ascii=False, indent=2) + "\n")

    save_record()
    print(f"Prepared input: {out / 'input-guide.png'}", flush=True)
    if args.stage == "prepare":
        return

    import torch
    torch.set_num_threads(max(1, args.threads))
    sys.path.insert(0, str(args.upstream))
    import tha3.util as util

    # Original release checkpoints are state dictionaries, not executable models.
    def load_weights(path):
        return torch.load(path, map_location="cpu", weights_only=True)

    util.torch_load = load_weights
    from tha3.poser.modes.separable_float import create_poser
    from tha3.poser.modes.pose_parameters import get_pose_parameters

    model_files = {name: str(args.models / (name + ".pt")) for name in MODULES}
    record["model_sha256"] = {name: digest(Path(path)) for name, path in model_files.items()}
    record["torch_version"] = torch.__version__
    poser = create_poser(torch.device("cpu"), module_file_names=model_files)
    parameters = get_pose_parameters()
    tensor = util.extract_pytorch_image_from_PIL_image(source)
    names = ["neutral", "blink100", "right012"] if args.stage == "keyframes" else list(POSES)
    frames = {}
    with torch.inference_mode():
        for name in names:
            path = out / "frames" / (name + ".png")
            pose = torch.zeros(poser.get_num_parameters())
            for key, value in POSES[name].items():
                pose[parameters.get_parameter_index(key)] = value
            started = time.monotonic()
            rendered = poser.pose(tensor, pose)[0].detach().cpu()
            rgba = util.rgba_to_numpy_image(rendered)
            frame = Image.fromarray(np.rint(np.clip(rgba, 0, 1) * 255).astype(np.uint8), "RGBA")
            frame.save(path)
            seconds = time.monotonic() - started
            frames[name] = frame
            record["frames"].append({"name": name, "pose": POSES[name], "seconds": round(seconds, 3),
                                     "path": "frames/" + path.name, "sha256": digest(path)})
            save_record()
            print(f"Rendered {name}: {seconds:.2f}s -> {path}", flush=True)

    comparison = Image.new("RGB", (1560, 450), "#f6f3e7")
    for index, (frame, label) in enumerate([
        (source, "原始立绘"), (frames["neutral"], "模型静止帧"),
        (frames["blink100"], "闭眼关键帧"), (frames["right012"], "侧头关键帧"),
    ]):
        comparison.paste(screen(frame, caption=label), (390 * index, 0))
    comparison.save(out / "comparison.png")
    if args.stage != "clips":
        print(f"Keyframe comparison: {out / 'comparison.png'}", flush=True)
        return

    for name, (sequence, durations) in CLIPS.items():
        images = [screen(frames[key]) for key in sequence]
        images[0].save(out / (name + ".png"), save_all=True, append_images=images[1:],
                       duration=durations, loop=0, format="PNG")
        images[0].save(out / (name + ".gif"), save_all=True, append_images=images[1:],
                       duration=durations, loop=0, disposal=2)
    record["clips"] = {name: {"sequence": seq, "durations_ms": durations}
                       for name, (seq, durations) in CLIPS.items()}
    save_record()
    (out / "index.html").write_text('''<!doctype html>
<html lang="zh-CN"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>苔苔 · 真实动作试片</title><style>
*{box-sizing:border-box}body{margin:0;padding:40px 24px;color:#354d32;background:radial-gradient(at 12% 0%,#e2e9cf,transparent 55%),#f5f1e7;font-family:"Noto Serif CJK SC",serif}
main{max-width:1240px;margin:auto}h1{font-size:32px;font-weight:500}p{line-height:1.9;max-width:850px}section{display:flex;flex-wrap:wrap;gap:24px;margin:32px 0}figure{margin:0;max-width:390px}figure img{width:100%;height:auto;border-radius:6px}figcaption{margin:12px 0;font-size:16px}button{font:inherit;border:1px solid #7b8c69;background:transparent;padding:8px 18px;cursor:pointer;color:inherit}.comparison{width:100%;height:auto}small{line-height:1.9;display:block}a{color:inherit}
</style><main><small>THA3 / 离线动作实验 / 未烧录</small><h1>让苔苔轻轻眨一下眼。</h1>
<p>保留原立绘，用神经网络生成眼睑与头部变化，不是整张图片上下移动。下方为 390 × 450 像素方形屏幕比例；窄屏会等比缩小，并非物理尺寸。请重点观察眼睛、脸型、兔耳及透明边缘。</p>
<button id="pause">暂停动作</button><section>
<figure><img src="original-screen.png" alt="未经模型处理的原立绘"><figcaption>原始立绘</figcaption></figure>
<figure><img class="motion" src="blink.png" data-clip="blink.png" alt="神经网络眨眼动作"><figcaption>眨眼 / 短暂闭合后恢复</figcaption></figure>
<figure><img class="motion" src="head-turn.png" data-clip="head-turn.png" alt="神经网络轻微侧头动作"><figcaption>轻微侧头 / 左右小角度</figcaption></figure></section>
<h2>关键帧对比</h2><img class="comparison" src="comparison.png" alt="原图、模型静止、闭眼、侧头对比">
<p>这是动画可行性试片，不代表最终美术验收。模型可能重绘细节或造成变形；板端帧率、存储占用、随机播放与功耗尚未验收。</p>
<small>模型：<a href="https://github.com/pkhungurn/talking-head-anime-3-demo">Talking Head Anime 3 · pkhungurn</a>，模型 CC BY 4.0 / 代码 MIT。变更：原图尺寸归一化、姿态驱动、裁切与屏幕合成。<a href="generation.json">生成参数与来源记录</a></small>
</main><script>let paused=matchMedia('(prefers-reduced-motion: reduce)').matches;const button=document.querySelector('#pause');function update(){document.querySelectorAll('.motion').forEach(img=>img.src=paused?'original-screen.png':img.dataset.clip);button.textContent=paused?'播放动作':'暂停动作'}button.onclick=()=>{paused=!paused;update()};update();</script></html>''')
    print(f"Motion preview: {out / 'index.html'}", flush=True)


if __name__ == "__main__":
    main()
