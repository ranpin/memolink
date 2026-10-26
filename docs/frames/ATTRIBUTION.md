# 帧来源与署名

demo 页（`index.html` 座舱助手 · 应用 tab）展示的来源帧均来自 **Wikimedia Commons**，
许可证逐个核实，**无 gated / 无版权不明数据**（铁律：不用 gated 驾驶帧，仓 public + Pages）。

| 文件 | 原始 Commons 页 | 作者 | 许可证 |
|---|---|---|---|
| `seat_rearfacing.jpg` | [File:Rear-facing infant car seat.jpg](https://commons.wikimedia.org/wiki/File:Rear-facing_infant_car_seat.jpg) | Pieter Kuiper | [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/) |

按 CC BY-SA 3.0 要求署名：作者 + 许可证 + 是否修改。上表即署名；帧未做内容修改
（仅作为 demo 的「摄像头观察」输入，按 `--image-max-tokens 300` 由 llama-server 内部缩放）。

这些帧同时是 `bench/cockpit_demo_script.json` 多模态轮次的输入（`cockpit_assistant observe`
在 Orin 上真实读取），与 demo 页展示的是同一帧。
