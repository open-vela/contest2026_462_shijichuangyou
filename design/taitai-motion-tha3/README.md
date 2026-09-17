# 苔苔 THA3 离线动作实验

本目录用于测试现有立绘的真实眼睑变化和轻微侧头，不改动固件，不使用整图晃动冒充角色动画。

## 来源与限制

- 原画：`assets/portraits/taitai-v1.png`，保持不变。
- 模型与代码作者：pkhungurn，https://github.com/pkhungurn/talking-head-anime-3-demo 。代码 MIT；预训练模型 CC BY 4.0。
- 模型来源：作者提供的 Dropbox `talking-head-anime-3-models.zip`，仅提取 `separable_float` 五个权重。
- 输入副本：512 × 512 RGBA，原图等比缩入 256 × 256，水平居中、顶边 y=8。红框是模型内侧面部参考区域，不是裁切原图。
- 输出展示：按黄山派 390 × 450 矩形屏幕比例合成，不作圆形裁切。
- 这些是模型修改后的实验图像，不保证保留全部原画细节。需查看关键帧后决定是否用于正式角色。
- 所有推理在本机 CPU 执行，立绘不上传云端。模型与运行环境保存在外接硬盘 `.tools/tha3-motion/`，不随比赛仓提交。

## 生成步骤

从比赛仓根目录运行，Python 使用外接盘专用虚拟环境：

```sh
PYTHON=../../.tools/tha3-motion/venv/bin/python
$PYTHON tools/generate_taitai_motion.py --stage prepare
$PYTHON tools/generate_taitai_motion.py --stage keyframes
# 确认关键帧适合后再执行完整样片：
$PYTHON tools/generate_taitai_motion.py --stage clips
```

`generation.json` 记录输入哈希、模型哈希、姿态参数和实际耗时。`comparison.png` 为原画、模型静止帧、闭眼、侧头对照。完整样片阶段才生成 `index.html`、APNG 和 GIF。

## 当前验收边界

有生成文件不代表美术合格。尚需确认面部、眼睛、兔耳和透明边缘的实际效果。板端资源转换、随机播放、帧率、存储占用、功耗、编译及烧录不属于本脚本已实现功能。
