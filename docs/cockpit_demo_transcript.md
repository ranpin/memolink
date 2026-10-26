# 座舱助手 demo 完整输出（Orin 真实转录）

> 这是 `cockpit_assistant run bench/cockpit_demo_script.json` 在 **Jetson Orin（dog）** 上的真实输出（2026-10-26 录制），是 [index.html](index.html#cockpit) 座舱助手演示分步回放的原始依据。全程离线：本地 bge-small-zh INT8 embedding + 本地 qwen3-4b（消解 / 提取 / 回复生成）+ 本地 Qwen3-VL-4B（多模态轮次的图像 caption，:8098）。除逐条 embedding 的基础设施日志外，输出原样保留。

**场景**：seed 40 条座舱记忆（owner=张三）→ 9 轮对话，逐个点亮 memolink 的独有机制——

1. **SUPERSEDE 版本化**：搬家后地址西湖区→滨江区，旧值失效保留、建版本链
2. **版本化检索**：问地址返回当前值（滨江区），失效旧值不再出现
3. **CREATE**：新事实（养柯基犬豆豆）入库
4. **episodic 类型特化检索**：「上次充电在哪」路由到 episodic
5. **procedural 类型特化检索**：「座椅加热怎么语音开」路由到 procedural
6. **多模态观察（Q9）**：摄像头帧 → VLM caption → 二次确认 → observed 记忆入库（标签文字复读不一致→降为 inferred）；摄像头观察取代口述（observed > asserted）
7. **视觉接地记忆参与检索**：问「后座装了什么」召回摄像头观察到的 observed 记忆
8. **多驾驶员治理拦截**：换驾驶员李四，张三的私人记忆（住址/孩子学校）不可见
9. **治理对照**：shared 记忆（车辆电池/续航）李四照常可见

```text
[seed] 库为空，已从 ../bench/corpus/cockpit.json 载入 40 条记忆（owner=张三）

════════════════ 第 1 轮 ════════════════
[驾驶员] 张三
[用户] 我搬家了，家庭地址从西湖区改成滨江区
[检索] 类型意图=semantic，治理=(owner=张三 ∪ scope=shared)
  # 3 [semantic/private/张三] score=1.102  用户住在杭州西湖区，家庭地址已存入导航常用地点。
  #26 [semantic/shared/张三] score=1.081  用户通勤路线固定：从西湖区到滨江，早高峰约 40 分钟。
  #12 [semantic/private/张三] score=0.693  用户的孩子在文三路上小学，每天 7:40 需要出发送学。
  #37 [semantic/shared/张三] score=0.689  用户希望助手回答简洁，不要啰嗦，重要信息优先。
[回复] 家庭地址已更新为滨江区。
[写入] 提取 1 条候选事实:
  [semantic] 「用户搬家，家庭地址从西湖区改为滨江区」 → SUPERSEDED id=41 sim=0.755 [System-2]
[版本链] SUPERSEDE 演化（旧→新）:
  # 3 [失效]  用户住在杭州西湖区，家庭地址已存入导航常用地点。
  #41 [有效]  用户搬家，家庭地址从西湖区改为滨江区

════════════════ 第 2 轮 ════════════════
[驾驶员] 张三
[用户] 我家现在住在哪个区？
[检索] 类型意图=semantic，治理=(owner=张三 ∪ scope=shared)
  #41 [semantic/private/张三] score=0.967  用户搬家，家庭地址从西湖区改为滨江区
  #26 [semantic/shared/张三] score=0.692  用户通勤路线固定：从西湖区到滨江，早高峰约 40 分钟。
  #38 [semantic/private/张三] score=0.613  用户的家庭Wi-Fi密码和公司门禁信息曾口头告知助手，属敏感。
  # 4 [semantic/shared/张三] score=0.612  空调偏好：夏季 22 度、风量 2 档，喜欢座椅通风。
[回复] 您现在住在滨江区。
[写入] 无可记事实（NOOP）

════════════════ 第 3 轮 ════════════════
[驾驶员] 张三
[用户] 我家里养了一只柯基犬叫豆豆，每天下班都要遛它
[检索] 类型意图=semantic，治理=(owner=张三 ∪ scope=shared)
  #20 [semantic/shared/张三] score=0.663  车机流量套餐每月 20G，本月已用 12G，主要是音乐和导航。
  #12 [semantic/private/张三] score=0.656  用户的孩子在文三路上小学，每天 7:40 需要出发送学。
  #18 [semantic/shared/张三] score=0.644  用户午餐常在车上解决，喜欢点清淡的简餐，不喝含糖饮料。
  #23 [semantic/private/张三] score=0.638  用户的紧急联系人是配偶，电话已存入车机紧急呼叫。
[回复] 好的，记住您家有一只叫豆豆的柯基犬。
[写入] 提取 2 条候选事实:
  [semantic] 「用户养了一只名为豆豆的柯基犬」 → CREATED id=42 sim=0.460 [System-1]
  [procedural] 「用户每天下班后都要遛豆豆」 → CREATED id=43 sim=0.446 [System-1]

════════════════ 第 4 轮 ════════════════
[驾驶员] 张三
[用户] 上次充电是在哪充的？
[检索] 类型意图=episodic，治理=(owner=张三 ∪ scope=shared)
  #10 [episodic/shared/张三] score=1.060  上次充电是在公司楼下快充站，充到 90%，花了 45 分钟。
  #36 [episodic/shared/张三] score=0.698  用户本周三预约了 4S 店的免费检测，时间是周六上午。
  # 5 [episodic/shared/张三] score=0.682  国庆假期用户计划自驾去千岛湖，已查询过路线和充电桩。
  #21 [episodic/shared/张三] score=0.648  今天早上用户出门晚了，让助手规划了最快路线并提前开启了空调。
[回复] 上次在公司楼下快充站充的。
[写入] 无可记事实（NOOP）

════════════════ 第 5 轮 ════════════════
[驾驶员] 张三
[用户] 座椅加热怎么用语音打开？
[检索] 类型意图=procedural，治理=(owner=张三 ∪ scope=shared)
  # 8 [procedural/shared/张三] score=1.279  用户习惯用语音打开座椅加热，说“把座椅加热开到三档”。
  #30 [procedural/shared/张三] score=0.963  用户习惯说“我有点冷”，助手应把空调温度调高两度并开座椅加热。
  #24 [procedural/shared/张三] score=0.810  用户会用“打开露营模式”让空调低功耗运行并关闭锁车提示音。
  #40 [procedural/shared/张三] score=0.725  用户说“太晒了”时，助手应关闭对应侧车窗并调低该侧空调出风。
[回复] 您可以说“打开座椅加热”来语音控制。
[写入] 无可记事实（NOOP）

════════════════ 第 6 轮 [摄像头] ════════════════
[驾驶员] 张三
[图像] ../docs/frames/seat_rearfacing.jpg
[caption] 车内安装了儿童安全座椅，座椅上固定着一名身穿红色衣服的幼儿，座椅靠背右上角有“PARENTA”字样，座椅安全带穿过幼儿身体并固定在车内。
[二次确认] 定向复读: 看不清
           ↳ 1 条事实的文字字段未获复读确认 → authority 降为 inferred（最低可信度，不覆盖任何记忆）
[写入] 提取 4 条候选事实（来源=cockpit-cam）:
  [semantic/observed] 「车内安装了儿童安全座椅」 → SUPERSEDED id=44 sim=0.707 [System-2] [authority升级]
      ↳ 观察(observed) 可信度高于口述(asserted)，UPDATE 升级为 SUPERSEDE 以保留版本链
  [semantic/observed] 「座椅上固定着一名身穿红色衣服的幼儿」 → CREATED id=45 sim=0.413 [System-1]
  [semantic/inferred] 「座椅靠背右上角有"PARENTA"字样」 → NOOP id=0 sim=0.503 [System-2] [文字未确认→降级]
  [semantic/observed] 「座椅安全带穿过幼儿身体并固定在车内」 → CREATED id=46 sim=0.447 [System-1]
[版本链] SUPERSEDE 演化（旧→新，标注 authority）:
  #31 [失效] [asserted]  后备箱常放着一个儿童安全座椅和一个折叠露营椅。
  #44 [有效] [observed]  车内安装了儿童安全座椅

════════════════ 第 7 轮 ════════════════
[驾驶员] 张三
[用户] 我后座装了什么？
[检索] 类型意图=semantic，治理=(owner=张三 ∪ scope=shared)
  #44 [semantic/shared/张三] score=0.784  车内安装了儿童安全座椅
  #46 [semantic/private/张三] score=0.676  座椅安全带穿过幼儿身体并固定在车内
  # 4 [semantic/shared/张三] score=0.621  空调偏好：夏季 22 度、风量 2 档，喜欢座椅通风。
  #18 [semantic/shared/张三] score=0.618  用户午餐常在车上解决，喜欢点清淡的简餐，不喝含糖饮料。
[回复] 后座装了儿童安全座椅。
[写入] 无可记事实（NOOP）

════════════════ 第 8 轮 ════════════════
[驾驶员] 李四
[用户] 导航到张三家的地址
[检索] 类型意图=semantic，治理=(owner=李四 ∪ scope=shared)
  #37 [semantic/shared/张三] score=0.732  用户希望助手回答简洁，不要啰嗦，重要信息优先。
  #26 [semantic/shared/张三] score=0.710  用户通勤路线固定：从西湖区到滨江，早高峰约 40 分钟。
  #27 [semantic/shared/张三] score=0.706  车辆软件版本 v3.2.1，上个月 OTA 升级后新增了城市领航辅助。
  #20 [semantic/shared/张三] score=0.694  车机流量套餐每月 20G，本月已用 12G，主要是音乐和导航。
[治理拦截] 无过滤本会命中、但对 李四 不可见的私人记忆:
  #41 [semantic/private/张三]  用户搬家，家庭地址从西湖区改为滨江区
  #12 [semantic/private/张三]  用户的孩子在文三路上小学，每天 7:40 需要出发送学。
[回复] 抱歉，我这边没有您张三家的地址记录。
[写入] 无可记事实（NOOP）

════════════════ 第 9 轮 ════════════════
[驾驶员] 李四
[用户] 这台车电池多大？满电能跑多少公里？
[检索] 类型意图=semantic，治理=(owner=李四 ∪ scope=shared)
  # 9 [semantic/shared/张三] score=1.208  车辆是纯电动 SUV，电池容量 100 度，满电续航约 620 公里。
  # 2 [semantic/shared/张三] score=0.819  车辆上次保养是 2026 年 8 月，里程 12000 公里。
  #16 [semantic/shared/张三] score=0.733  轮胎是 255/45 R20，胎压建议 2.5 bar，上次补胎是 3 月。
  #11 [semantic/shared/张三] score=0.713  用户不喜欢车内氛围灯太亮，通常调到 30% 暖白色。
[回复] 车辆是纯电动 SUV，电池容量 100 度，满电续航约 620 公里。
[写入] 无可记事实（NOOP）
[dump] 10 个快照 → ../bench/cockpit_demo_store.json

════════════════ 结束 ════════════════
最终库: total=46 valid=44 invalid=2
```

**注 1**：`[检索]` 里的 `score=` 是 `recallTyped` 的**排序分** = 余弦相似度 × 同类型加权（记忆类型命中查询意图时 ×1.6）× episodic 时间衰减（exp(-age_days/365)）——**不是裸余弦**，乘了加权所以可以大于 1（如第 5 轮 #8 的 1.279 ≈ 裸余弦 0.799 × 1.6）。它只用于排序，不当相似度解读。

**注 2**：第 8 轮的 `[治理拦截]` 是 demo 标注——展示「无过滤本会命中、但被治理过滤掉」的私人记忆；实际 `recallTyped` 对李四只返回 `scope=shared` 的记忆。治理是检索期访问控制（fail-closed strict 模式），不是加密，详见 [memory_v2_design.md](memory_v2_design.md) §8。

**注 3**：第 6 轮是 Q9 多模态轮次。`[caption]` 是 Qwen3-VL-4B 对摄像头帧的描述；`[二次确认]` 是对同一帧的中性定向复读，文字字段若复读不一致（如标签「PARENTA」复读答「看不清」）则该事实 authority 降为 inferred（最低可信度，不覆盖任何记忆）。第 6 轮的 `[authority升级]` 标注表示摄像头观察（observed）取代了驾驶员口述（asserted）——observed 可信度更高。来源帧见 [frames/seat_rearfacing.jpg](frames/seat_rearfacing.jpg)（许可证见 [frames/ATTRIBUTION.md](frames/ATTRIBUTION.md)）。
