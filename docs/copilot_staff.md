# Copilot 员工登记表

## 用途

用于记录不同 Copilot 当前负责的工作、进度、阻塞和交接信息。

## 填写规则

- 每个 Copilot 占用一个独立小节。
- 新增成员时，直接复制下方模板。
- 内容尽量简短，优先写清楚“负责什么、做到哪了、卡在哪、下一步是谁接”。
- 已完成的工作不要删除，保留记录。

---

## 成员总览

| Copilot 名称 | Resume | 负责模块 | 当前状态 | 最新进展 | 阻塞 | 接手人 |
| --- | --- | --- | --- | --- | --- | --- |
| Copilot CLI（GPT-5.4） | `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | 项目结构、构建入口、沙盒、地面敌人、飞行敌人原型、Boss 原型 / `CombatSystem`、参数文档、`module_02 / module_04` 地图细化 | 进行中 | 已认领并行任务 B：Boss 原型 / 敌人扩展 / 战斗结算层最小化；此前已完成地面怪与飞行怪正式模块化并接入主循环，补上击杀 +10 HKD / `+10` 飘字，并细化 `module_02`、`module_04` 战斗白盒地图 | 本轮禁止改 `main.cpp`，后续需主流程位预留 Boss 刷入与正式接线口 | 下一位接手主流程接线或继续补完整 Boss/Battle loop 的 Copilot |
| Copilot CLI（GPT-5.4） | 待填写（当前会话 resume 退出后补） | 技能/攻击 ASCII 表现设计、法术分镜、显示语义约定 | 进行中 | 已补充左右冲击波、下砸、普通攻击缺口、敌人受伤/死亡、HKD/HUD 与 `YOU DEAD` 白屏死亡演出的 ASCII 方案 | 无 | 下一位接手技能正式渲染接入、玩家攻击判定或死亡演出落地的 Copilot |
| Copilot CLI（GPT-5.4） | `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e` | 玩家沙盒、敌人沙盒、MapDrawer 渲染、参数同步、主线 Player 接入、`module_01 / module_03` 地图细化、主菜单 / 存档最小闭环 | 已完成 | 已完成玩家攻击/法术/HUD/`YOU DEAD` 死亡演出接入主线 `Player.cpp`，改成 `W/S` + `J/K` 组合键映射，并细化 `module_01`、`module_03`；现已补完 `NEW GAME` / `CONTINUE` / 最小存档闭环，主线可保存地图、出生点、HP、Soul、HKD，并在死亡后按当前存档点恢复 | 无 | 下一个负责玩家状态机、击退、主线战斗、固定存档点或继续扩图的人 |
| Copilot CLI（GPT-5.4） | ` copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb` | 参数整理、调参文档、员工文档、进度盘点、地图仓库、`NpcSystem` 最小交互、主界面 HUD 精简 | 进行中 | 已完成基础参数归档、员工登记表建立与 resume 对应、8 张白盒地图 + `testcpp1` 主线接线、地图分工表；已细化 `spawn_village` / `random_event_01`，修复 `module_02` 入口卡墙与 `module_01` 边界破损；已落地 `NpcSystem` 最小交互模块，并把主界面改成顶部极简 HUD + 底部常驻操作说明，现已把 `NpcSystem` 正式接进 `main.cpp` 运行时，支持 `E` 交互 / `W/S` 选项 / `J` 购买，并补齐 `Makefile` / `run.bat` 构建项 | 待继续做整线跑图验证，确认各 NPC / 门 / 存档点在真实流程里都稳定 | 下一位接手整线跑图验证、UI 微调或继续补文档的 Copilot |

---

## 工作登记表格

用于按“这一次干了什么”来登记，不替代下面的成员长期档案。

| 日期 | Copilot / 负责人 | 工作模块 | 当前状态 | 本轮完成 | 下一步接手 | 备注 |
| --- | --- | --- | --- | --- | --- | --- |
| 2026-04-21 | Copilot CLI（GPT-5.4） | 地图系统文档 / Staff 文档 | 已完成 | 补充世界布局地基文档，新增 staff 工作登记表格模板 | 后续接手地图系统接线或继续扩图的 Copilot | 当前只定结构，不提前细画地图 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | 白盒地图 / 主程序接线 / Staff 分工 | 已完成 | 落地 8 张白盒地图并接入 `testcpp1`，补充 4 个 AI 的地图分工表 | 各地图负责 AI 按分工逐张细化 | 当前阶段主任务就是按房间逐张刻画 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | 并行任务 B 认领 | 进行中 | 已认领 Boss 原型 / 敌人扩展 / 战斗结算层最小化；本轮职责锁定在 `Enemy.*` 与 `CombatSystem.*` | 后续由本会话继续补 Boss 原型与最小战斗结算接口 | 暂不触碰 `src\core\main.cpp`、存档和 NPC 文件 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | `module_02` / `module_04` 地图细化 | 已完成 | 把两张战斗图改成更适合当前跳跃与战斗的多层白盒，补到 5 只敌人并保留充足走位空间 | 下一位接手主程序跑图验证或继续补战斗结算的 Copilot | 重点满足“可跳、怪适中、结构更复杂、空间不卡手” |
| 2026-04-21 | Copilot CLI（GPT-5.4） 待填写（当前会话 resume 退出后补） | `module_05` / `boss_room_01` 地图细化 | 已完成 | 已细化 Boss 前通道与 Boss 房平台层次，补充多只地面怪 / 飞行怪站位，并校正行宽与落点可用性 | 下一位接手主程序跑图验证或继续补 Boss 正式内容的 Copilot | 当前版本仍是白盒，但已满足可跳、可战斗、结构更复杂的要求 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | `spawn_village` / `random_event_01` 细化 | 已完成 | 把出生村改回纯安全区并保留多层 NPC 动线；把事件房改成可上下折返的多层事件战斗房，并补上适中偏多的地面怪 / 飞行怪 | 其他地图负责 AI 继续按同标准细化各自两张图 | 重点保证平台可跳、空间不挤、结构比首版白盒更像《空洞骑士》 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | Staff / 计划同步 | 已完成 | 更新当前会话档案，补记 `spawn_village` / `random_event_01` 已细化完成，并记录 active map 行宽与 transition 已重新校验通过 | 下一位接手整线流程验证或继续补剩余地图细化的 Copilot | 该文员位当前重点从“建仓”转为“进度同步 + 地图交接” |
| 2026-04-21 | Copilot CLI（GPT-5.4） | 第二张图卡住修复记录 | 已完成 | 定位到 `module_02` 入口出生点刷进墙体、`module_01` 边界破损和坏刷点；修正后 active map 再校验通过，用户确认第二张图已恢复正常 | 下一位接手整线跑图验证或继续查后续房间落点问题的 Copilot | 说明问题来自地图数据，不是主循环逻辑卡死 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | NPC / 医生 / 商人 / 商店最小交互 | 已完成 | 已完成 `include\\npc\\NpcSystem.h` 重构与 `src\\npc\\NpcSystem.cpp` 新建，并已把结果正式接进 `src\\core\\main.cpp`；现在靠近 NPC 时 `E` 可触发真实交互，商店支持 `W/S` 选商品、`J` 购买、`E` 关闭，并同步补齐 `Makefile` / `run.bat` 的 `NpcSystem` 构建项 | 下一位接手整线跑图验证或继续做商店 / 事件正式逻辑的 Copilot | 本轮已不再停留在提示文案，运行时交互与存档同步都已接上 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | NPC 运行时接线修复 | 已完成 | 修复底部出现 `Press E` 但按下后没有真实效果的问题；主程序现已接入医生回血、村长对话、商人开店与购买结果，并在交互后同步当前存档数据 | 下一位接手整线跑图验证或继续补交互 UI 的 Copilot | 额外补齐 `Makefile` / `run.bat`，避免后续重新编译时漏链 `src\\npc\\NpcSystem.cpp` |
| 2026-04-21 | Copilot CLI（GPT-5.4） | 紧急任务 2：主界面 HUD 精简 | 已完成 | 只在允许范围内调整 `Player` HUD 与 `main.cpp` 的拼接，把顶部显示收缩到灵魂 / 血量 / HKD / 地点，并去掉大段教学与调试信息 | 下一位接手继续做整线跑图验证或 UI 微调的 Copilot | 本轮未改敌人、存档和 NPC 模块逻辑 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | HUD 交互提示回补 | 已完成 | 在底部恢复常驻基础操作说明，并让靠近门 / NPC 时追加显示 `Press E` 交互提示 | 下一位接手继续做 UI 微调或整线跑图验证的 Copilot | 保持顶部 HUD 简洁，不回退到旧调试屏 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e` | 并行任务 A：主菜单 / 存档最小闭环 | 已完成 | 接通 `GameSession` / `SaveSystem` / `main.cpp`，让标题页按真实存档显示 `CONTINUE`，新游戏会落盘，切图自动存档，死亡后按当前存档点恢复 HP / Soul / HKD / 地图与出生点，并补齐 `Makefile` / `run.bat` 构建项 | 下一位接手固定存档点、难度选择或更完整的流程 UI 的 Copilot | 已用最小存档烟测确认存读与死亡恢复链路可用 |

---

## 当前主任务

是的，**现在的主要工作就是一张一张图去刻画**。

当前优先级：

1. 继续把剩余白盒房间按同标准细化：平台可跳、敌人适中、门洞与动线清晰
2. 用 `testcpp1` 做一遍从 `spawn_village` 到 `boss_room_01` 的整线跑图验证
3. 主流程稳定后，再补捷径门状态、事件房和商店房的正式逻辑

---

## 当前开发纵览

现在项目已经不再是“散乱原型”阶段，**核心主链其实已经收得比较拢了**。

当前已基本成型的部分：

1. 地图仓库、地图加载、主程序地图驱动流程
2. 玩家移动 / 跳跃 / 普攻 / 法术 / 回血 / 死亡表现
3. 地面敌人 / 飞行敌人 / 击杀 +10 HKD / 基础 HUD
4. 8 张主线白盒地图与切图流程
5. 标题界面、`NEW GAME` / `CONTINUE` 与最小存档闭环

因此，**离一个可交付 demo 剩下的内容已经不算多了**。当前真正还缺、且直接影响 demo 完整度的核心块，主要只剩：

1. 两扇快捷捷径门：先锁、绕远、另一侧解锁、之后双向通行
2. HUD 精简与 soul 单格重绘修复
3. `NpcSystem` 正式接入主流程，让村长 / 医生 / 商人真正可交互
4. 商店最小消费闭环，让 HKD 真正有用途
5. Boss 原型与 `boss_room_01` 的正式战斗闭环
6. 全流程再跑一遍，清掉整线卡点、切图和落点问题

结论：

- **当前更像“收尾 + 接线 + 打磨”阶段**，不是大面积新开荒阶段
- 接下来优先级不该再分散，而应围绕“把现有主链闭合成 demo”推进
- 当前 demo 完成度可按 **约 70%** 理解；若上述 6 项补齐，项目就会从“能玩的原型”进入“成型 demo”

---

## 下一轮并行开发任务下发（必须分开开发）

当前要求：**另外 3 个 AI 必须按模块拆开并行开发，不允许多人同时改同一组核心文件。**

并行规则：

1. 每位 AI 只负责自己分到的模块和文件组，不跨线抢改别人的模块。
2. `main.cpp` / 主流程接线只允许**一个** AI 负责，其他 AI 先把各自模块做成可接入接口。
3. 如果确实需要改到别人的文件，先在 staff 里补一句“需要对方预留 hook / 接口”，不要直接覆盖。
4. 当前阶段优先做“能接入 demo 的最小可用版本”，不要提前做大重构。

### 并行任务 A：主菜单 / 新游戏继续游戏 / 存档最小闭环

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e`

**目标**

- 把程序启动流程补成真正的游戏入口
- 做出 `NEW GAME` / `CONTINUE` 的最小可用闭环
- 接上最小存档，让跑图流程不再只是测试态

**只负责这些文件**

- `src\core\main.cpp`
- `include\core\GameSession.h`
- `src\core\GameSession.cpp`（新建）
- `include\save\SaveSystem.h`
- `src\save\SaveSystem.cpp`（新建）

**本轮交付**

- 标题界面稳定进入主流程
- `新游戏` 能正常初始化 run
- `继续游戏` 能读取最小存档
- 至少保存：当前地图、出生点、HP、Soul、HKD
- 死亡后最小恢复逻辑与当前地图流程一致

**禁止触碰**

- `src\enemy\Enemy.cpp`
- `include\enemy\Enemy.h`
- `include\npc\NpcSystem.h`
- `src\npc\NpcSystem.cpp`

### 并行任务 B：Boss 原型 / 敌人扩展 / 战斗结算层最小化

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af`

**目标**

- 先补一个能打的 Boss 原型
- 顺手把敌人/Boss 这侧的战斗结算继续从临时逻辑往正式模块推进
- 重点是 Boss 房不再空着，能形成终点战斗目标

**只负责这些文件**

- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `include\combat\CombatSystem.h`
- `src\combat\CombatSystem.cpp`（新建）

**本轮交付**

- 至少 1 个 Boss 原型可刷进 `boss_room_01`
- Boss 至少有：血量、受击、死亡、1~2 个攻击、基础节奏
- 敌人 / Boss 击杀奖励与伤害结算尽量往 `CombatSystem` 收
- 不要求一次做完整 Boss 树，但要能形成可验证战斗

**禁止触碰**

- `src\core\main.cpp`
- `include\save\SaveSystem.h`
- `src\save\SaveSystem.cpp`
- `include\npc\NpcSystem.h`
- `src\npc\NpcSystem.cpp`

### 并行任务 C：NPC / 医生 / 商人 / 商店最小交互

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb`

**目标**

- 让出生村从“有站位”变成“有最小交互”
- 先把村长、医生、商人做成可消费/可提示/可回血的基础内容
- 让 HKD 真正开始有用途

**只负责这些文件**

- `include\npc\NpcSystem.h`
- `src\npc\NpcSystem.cpp`（新建）
- 如确实需要再改：`data\maps\spawn_village.map`
- 文档同步：`docs\game_design.md`、`docs\parameter_tuning.md`

**本轮交付**

- 村长：至少一句主线提示
- 医生：交互后回满血
- 商人：最小商店界面
- 商店里至少 2~3 个商品占位，例如加血、加攻、黑冲/二段跳预留
- HKD 消耗逻辑最小可用

**禁止触碰**

- `src\core\main.cpp`
- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `include\save\SaveSystem.h`
- `src\save\SaveSystem.cpp`

---

## 当前紧急插单任务（继续并行、禁止抢文件）

这 3 个问题优先级高于一般扩展内容，**直接影响当前主线可玩性**。  
要求：仍然按模块拆开做，不允许多人同时改同一组核心文件。

### 紧急任务 1：快捷捷径门改成“先锁、绕路、另一侧解锁、之后双向通行”

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e`

**目标**

- 把 `module_03 <-> spawn_village`、`module_05 <-> spawn_village` 两组捷径门改成真正的单向解锁门
- 初始状态：出生村侧不可用
- 玩家从远路绕到另一侧后，可在门前交互解锁
- 解锁后该门永久双向可用，并写入存档

**只负责这些文件**

- `src\core\main.cpp`
- `include\save\SaveSystem.h`
- `src\save\SaveSystem.cpp`
- `include\core\GameSession.h`
- `src\core\GameSession.cpp`

**如确实需要再改**

- `data\maps\spawn_village.map`
- `data\maps\module_03.map`
- `data\maps\module_05.map`

**本轮交付**

- 两扇捷径门都有锁定 / 解锁状态
- 解锁前出生村侧不能直接通行
- 解锁动作必须从远侧触发
- 解锁后退出再进游戏仍保持开启

**禁止触碰**

- `src\player\Player.cpp`
- `include\player\Player.h`
- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`

### 紧急任务 2：主界面 HUD 精简

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb`

**目标**

- 把当前主程序顶部信息压缩成极简 HUD
- 只保留：
  - 灵魂
  - 血量
  - HKD
  - 地点
- 去掉当前过多的动作提示、调试提示、冗余说明

**只负责这些文件**

- `src\player\Player.cpp`
- `include\player\Player.h`

**如确实需要再改**

- `src\core\main.cpp` 中 HUD 拼接相关的极小范围代码，但必须只改显示拼接，不动主流程逻辑

**本轮交付**

- 进入游戏后顶部 HUD 明显更简洁
- 不再出现一长串教学/调试信息挤占画面
- 地点名必须可见

**禁止触碰**

- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `include\save\SaveSystem.h`
- `src\save\SaveSystem.cpp`
- `include\npc\NpcSystem.h`
- `src\npc\NpcSystem.cpp`

### 紧急任务 3：Soul 单个容器重绘修复

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af`

**目标**

- 修复 soul 容器单个重绘异常
- 保证容器在主程序 HUD 里不会出现单格错位、覆盖、残影或状态显示不准
- 只做最小修复，不顺手扩成大规模 HUD 重构

**只负责这些文件**

- `src\player\Player.cpp`
- 如确实需要再改：`src\world\MapDrawer.cpp`

**本轮交付**

- 单个 soul 容器显示稳定
- 切图、受伤、回血、施法后重绘不乱
- 不破坏现有 HP / HKD / 地点显示

**禁止触碰**

- `src\core\main.cpp`
- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `include\npc\NpcSystem.h`
- `src\npc\NpcSystem.cpp`

说明：

- 任务 2 和任务 3 都会碰 `Player.cpp`，因此**必须顺序执行**：
  1. 先由任务 2 完成 HUD 精简并提交
  2. 再由任务 3 基于最新版本只修 soul 单格重绘
- 任务 1 可与任务 2 / 3 完全并行

---

## 当前地图分工（4 个 AI / 8 张图）

| Copilot / Resume | 分配地图 | 负责重点 | 交付要求 |
| --- | --- | --- | --- |
| Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e` | `module_01`、`module_03` | 主线前段推进手感、门洞位置、节点房切图可读性 | 让玩家从出生点出来后的第一段流程最顺，特别是 `module_03` 的多方向出口要一眼看懂 |
| Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | `module_02`、`module_04` | 战斗白盒、敌人站位、地面怪 / 飞行怪压测空间 | 保证这两张图既能打又不卡手，敌人刷点和平台高度要适合当前战斗系统 |
| Copilot CLI（GPT-5.4） `copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb` | `spawn_village`、`random_event_01` | 出生村层次、NPC / 事件占位、支线房折返路径、事件房敌人密度 | 保证 `spawn_village` 保持安全区属性，同时让平台可跳、空间宽松、NPC/事件点明确，并把白盒复杂度往《空洞骑士》质感上提一档 |
| Copilot CLI（GPT-5.4） 待填写（当前会话 resume 退出后补） | `module_05`、`boss_room_01` | Boss 前氛围、终点房净空、整体视觉节奏 | 把 Boss 前通道和 Boss 房的“决战感”做出来，但仍保持白盒简洁 |

---

## 成员模板

### Copilot：`名称待填写`

**负责模块**

- 待填写

**工作目标**

- 待填写

**当前状态**

- 未开始 / 进行中 / 已完成 / 阻塞

**已完成**

- 待填写

**进行中**

- 待填写

**待处理**

- 待填写

**阻塞 / 风险**

- 无 / 待填写

**需要同步的参数 / 规则**

- 无 / 待填写

**交接说明**

- 待填写

**最后更新**

- 日期：待填写
- 填写人：待填写

---

## 成员记录

### Copilot：`Copilot CLI（GPT-5.4）`

**Resume**

- `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af`

**负责模块**

- 项目目录重构（`src/`、`include/`、`docs/`）
- Windows 构建 / 启动入口（`Makefile`、`run.bat`、`run_sandbox.bat`）
- Sandbox 体系接入与维护
- 地面普通敌人原型与 EnemySandbox 接线
- 飞行敌人简化原型与调参文档同步
- Boss 原型 / 敌人扩展 / `CombatSystem` 最小化

**工作目标**

- 把当前控制台原型整理成可维护的模块结构
- 提供稳定的沙盒调试入口，方便单独测试玩家 / 技能 / 敌人
- 先做出“能玩、能调、能继续接”的普通敌人原型
- 给 `boss_room_01` 补上最小可验证的 Boss 战斗目标
- 把敌人 / Boss 的伤害与奖励结算从临时逻辑继续往 `CombatSystem` 收口

**当前状态**

- 进行中

**已完成**

- 修复早期 Windows 可执行文件与运行环境问题
- 优化控制台刷新方式，减轻闪屏
- 完成项目目录重构并更新构建脚本
- 接入 Player / Skill / Combat / Enemy / World 沙盒入口
- 完成地面普通敌人：巡逻、索敌、`!!!` 预警、Dash 攻击、恢复、脱战回位
- 完成飞行敌人简化版：慢速悬停、前摇、火球攻击、1 点伤害
- 完成地面普通敌人正式模块化，并接入当前主运行链路
- 完成飞行敌人正式模块化，并接入 EnemySandbox 与当前主运行链路
- 完成击杀地面敌人后 +10 HKD 结算与玩家头顶 `+10` 飘字
- 完成 Player HUD 的 HKD 显示接入
- 完成 `module_02`、`module_04` 两张战斗白盒地图细化，提升平台层次、敌人站位和走位空间
- 更新 `docs/game_design.md` 与 `docs/parameter_tuning.md`
- 已把阶段性代码推送到 GitHub

**进行中**

- 已认领并行任务 B：Boss 原型 / 敌人扩展 / 战斗结算层最小化
- 接下来在 `include\enemy\Enemy.h`、`src\enemy\Enemy.cpp`、`include\combat\CombatSystem.h`、`src\combat\CombatSystem.cpp` 内补最小可接入版本
- 保持参数文档和当前实装行为一致

**待处理**

- Boss 原型：血量、受击、死亡、至少 1~2 个攻击与基础节奏
- `CombatSystem` 最小化：统一敌人 / Boss 的伤害、击杀奖励和后续可接线接口
- 地面 / 飞行敌人的正式掉落、统一奖励结算、血条 UI
- 玩家正式攻击判定、受伤击退、无敌帧与敌人系统接通
- EnemySandbox 补成更完整的调试场

**阻塞 / 风险**

- 本轮按 staff 要求不能改 `src\core\main.cpp`，所以 Boss 刷入和主流程正式接线需要主流程位后续配合
- 玩家正式战斗链路还没完全接好，所以很多敌人 / Boss 行为仍需先在敌人模块和最小结算层内自洽
- `CombatSystem` 这次先做最小闭环，不在这一轮提前扩成完整大系统

**需要同步的参数 / 规则**

- 玩家当前移动 / 跳跃参数记录在 `docs/parameter_tuning.md`
- 地面敌人当前参数包括：血量 3、索敌 10、脱战 16、巡逻半径 3、预警 0.35 秒、Dash 攻击范围 2、恢复 1 秒、击杀奖励 10 HKD
- 飞行敌人当前参数包括：血量 2、索敌 12、脱战 18、预警 0.30 秒、恢复 0.80 秒、悬停范围 2、移动步进 0.24 秒、火球伤害 1、击杀奖励 10 HKD
- 当前击杀地面敌人后，玩家头顶会短暂显示 `+10` 飘字
- `module_02`、`module_04` 当前按战斗白盒思路细化：平台必须可跳、敌人数量适中偏多、结构更接近《空洞骑士》式多层空间
- 并行任务 B 当前只允许动 `Enemy.*` 与 `CombatSystem.*`，不直接改 `main.cpp` / 存档 / NPC 侧文件

**交接说明**

- 如果下一位要接主流程，请先给 Boss 刷入 `boss_room_01` 和 `CombatSystem` 调用预留 hook，再接正式战斗入口
- 如果下一位要继续做敌人，就优先补 Boss 原型的完整攻击节奏、统一掉落、奖励发放和主线战斗结算
- 如果下一位要先做玩家战斗，就优先把玩家攻击、受伤、无敌帧与主程序 / EnemySandbox / 敌人模块彻底打通，并把临时写在 Player 里的击杀奖励逻辑迁到中间结算层
- 文档更新时，优先同步 `docs/game_design.md`、`docs/parameter_tuning.md`、`docs/module_checklist.md`

**最后更新**

- 日期：2026-04-21
- 填写人：Copilot CLI（GPT-5.4）

---

### Copilot：`Copilot CLI（GPT-5.4）`

**Resume**

- 待填写（退出后补当前 `copilot --resume=...`）

**负责模块**

- `docs/parameter_tuning.md` 参数整理与维护
- `docs/copilot_staff.md` 员工文档建立与补录
- 项目当前完成度、sandbox 存在性、模块缺口盘点
- 地图仓库目录、索引与示例地图接入
- `include\npc\NpcSystem.h` / `src\npc\NpcSystem.cpp`
- 出生村 NPC / 医生 / 商人 / 商店最小交互
- `src\player\Player.cpp` / `include\player\Player.h` 主界面 HUD 精简

**工作目标**

- 作为文员整理今晚口述的数值参数与机制
- 建立 Copilot 员工登记表，方便多位 Copilot 交接
- 帮你快速确认当前项目做到哪里、还缺什么

**当前状态**

- 进行中

**已完成**

- 新建 `docs/parameter_tuning.md`
- 记录并整理基础移动、跳跃、灵魂、凝聚回血、攻击伤害参数
- 按你的要求删去游泳和复杂 HUD/动画参数，收敛成基础调参文档
- 新建 `docs/copilot_staff.md`
- 补录 enemy Copilot 与 player sandbox Copilot 的 resume
- 盘点当前 sandbox 缺口与项目整体完成度
- 新建 `data\maps` 地图仓库、索引文件和 3 张示例地图
- 为 `MapLoader` / `WorldSandbox` 接上真实地图加载路径
- 把 `WorldSandbox` 从占位输出改成可自由移动/跳跃的一张地图漫游沙盒
- 给出征点村庄补上老人、医生、可进入的小房子，以及房内商人交互占位
- 新建 `MapEditorSandbox`，支持放平台、放敌人、设出生点并一键保存成 `.map`
- 新建 `docs/world_layout_foundation.md`，整理世界坐标表、连接表、捷径门规则和 8 张地图的结构骨架
- 已把 8 张白盒地图接入 `testcpp1`，主程序已改为从 `spawn_village` 开始的地图驱动流程
- 已在 staff 中下发 4 个 AI / 8 张图的当前地图分工
- 已细化 `spawn_village` 与 `random_event_01`，其中 `spawn_village` 保持无敌人的安全区，`random_event_01` 补上可跳平台、适中偏多的敌人和更明确的事件点位
- 已校正 `spawn_village` 与 `module_03` 的 active map 行宽问题，当前 active map 集合 transition / spawn 校验已通过
- 已修正 `module_02` 入口出生点刷进墙体、坏刷点和 `module_01` 边界破损；用户已确认第二张图恢复正常
- 已重构 `include\npc\NpcSystem.h` 并新建 `src\npc\NpcSystem.cpp`
- 已把村长主线提示、医生回满血、商店报价与购买结果收进 `NpcSystem`
- 已同步 `docs\game_design.md` 与 `docs\parameter_tuning.md` 的 NPC / 商店最小闭环规则
- 已按紧急任务 2 精简主界面 HUD，主程序顶部只保留灵魂、血量、HKD 和地点
- 已把 `NpcSystem` 正式接进 `src\core\main.cpp` 运行时，`E` 可触发真实 NPC 交互
- 已补上商店内 `W/S` 选项、`J` 购买、`E` 关闭，并在交互后同步存档侧数据
- 已补齐 `Makefile` / `run.bat` 的 `src\npc\NpcSystem.cpp` 构建项，避免主程序重新编译时断链

**进行中**

- 继续同步 staff、计划和地图交接状态
- 等待其他地图负责 AI 的细化结果，便于统一收口和总盘点
- 后续补一轮 `testcpp1` 的整线跑图记录，重点确认出生村 NPC 交互在真实流程里稳定

**待处理**

- 退出后由你补上当前会话的 resume
- 后续若再有新参数，继续同步到 `docs/parameter_tuning.md`
- 后续补一轮 `testcpp1` 的整线跑图记录
- 后续根据主流程验证结果，继续维护 staff 分工和地图交接说明
- 后续继续排查其余房间是否还有刷点压墙或边界破损的同类问题
- 如有需要，再对极简 HUD 做轻量排版微调，但不回退到旧调试屏

**阻塞 / 风险**

- 我这里无法直接读取当前会话对应的 resume 编号，只能由你在退出后补填

**需要同步的参数 / 规则**

- 当前基础调参以 `docs/parameter_tuning.md` 为准
- Copilot 分工与交接以 `docs/copilot_staff.md` 为准
- 地图文件仓库当前位于 `data\maps`
- NPC / 商店最小闭环规则当前以 `include\npc\NpcSystem.h`、`src\npc\NpcSystem.cpp` 和相关文档为准

**交接说明**

- 明天如果继续做文档/参数整理，优先维护 `docs/parameter_tuning.md`
- 如果继续整理多人协作记录，优先维护 `docs/copilot_staff.md`
- 如果继续做地图系统，优先从 `include\world\WorldSystem.h`、`src\world\WorldSystem.cpp`、`data\maps` 开始
- 当前这位现在除“文员位 + 地图仓库搭建”外，也负责地图总盘点、分工下发、主线白盒推进对齐和地图细化进度同步
- 本轮 `NpcSystem` 已正式接进主程序；后续主要是整线验证和交互 UI 微调，不再是“只做模块、不接主线”

**最后更新**

- 日期：2026-04-21
- 填写人：Copilot CLI（GPT-5.4）

### Copilot：`Copilot CLI（GPT-5.4）`

**Resume**

- 待填写（当前会话 resume 退出后补）

**负责模块**

- 技能 / 攻击的 ASCII 表现设计
- 法术分镜、视觉节奏与字符语义约定
- 现有控制台地图风格下的技能显示方案
- `module_05` / `boss_room_01` 白盒地图细化

**工作目标**

- 给当前项目补出可直接落地的技能视觉设计稿
- 统一“攻击判定长什么样、特效字符怎么表达”的规则
- 让后续技能实装时能按同一套 ASCII 语言继续扩展
- 把分配到的 Boss 前通道和 Boss 房白盒推进到“可跳、可战斗、可跑图验证”的状态

**当前状态**

- 进行中

**已完成**

- 阅读 `SkillSystem`、`CombatSystem`、`MapDrawer` 与相关 sandbox，确认当前仓库尚未实现具体技能帧
- 确认当前渲染模式适合“固定尺寸、整帧覆盖重绘”的 ASCII 动画
- 完成 `SoulWaveUp` / “向上吼叫”法术的 ASCII 分镜设计
- 完成左右法术冲击波（横向黑波）的极简 ASCII 方案
- 完成空中下砸 / 黑砸的 ASCII 分镜设计
- 盘点普通攻击动画缺口，确认左右劈、上劈、下劈尚未正式接入表现层
- 完成敌人受伤 / 死亡的极简 ASCII 反馈方案，收敛为短闪与快速消散
- 完成 HKD 入账、玩家 HUD（三属性）与死亡后 `YOU DEAD` 白屏扩张演出的表现设计
- 完成 `module_05` / `boss_room_01` 两张地图细化：补足可跳平台层次、增加适中偏多的地面怪 / 飞行怪，并保留门洞与跑图空间
- 约定关键字符语义：`!` 作为主伤害柱，`*` 作为爆裂核心，`^` 作为上冲尖端，`.` 作为残余魂粒子

**进行中**

- 按你的提问继续细化其他技能、HUD、死亡反馈与白盒地图表现方案

**待处理**

- 把技能设计稿正式接进 sandbox 或渲染层
- 为不同技能补齐统一的帧节奏、判定范围和字符规范
- 把“表现设计”继续扩展到普攻、上劈、下砸、黑冲、治疗、玩家死亡与敌人死亡等动作
- 如需继续负责地图侧，配合主程序跑图验证 `module_05` / `boss_room_01` 的节奏、刷怪密度与 Boss 前氛围

**阻塞 / 风险**

- 当前仓库只有技能枚举和接口，尚无正式技能表现层；后续接代码时需要先补渲染挂点
- 地图细化目前以白盒为主，是否继续加门锁、Boss 触发和更复杂结构，还要看主程序流程验证结果

**需要同步的参数 / 规则**

- 技能显示应保持固定宽高，避免控制台动画抖动
- 技能特效优先锚定玩家位置，再向目标方向扩展
- 字符语义要在不同技能间复用，避免每个技能都使用不同视觉语言

**交接说明**

- 如果下一位 Copilot 要把这套设计落地，优先从 `include\combat\SkillSystem.h`、`src\sandbox\SkillSandbox.cpp`、`src\world\MapDrawer.cpp` 开始
- 如果下一位继续做文案式技能设计，沿用当前“固定尺寸 + 底部锚定 + 多帧分镜”的方法
- 向上法术可以直接参考这次已经定义的“蓄力 -> 细柱 -> 炸冠 -> 消散”结构
- 横向冲击波、空中下砸、敌人死亡和 `YOU DEAD` 白屏都已经有可直接落地的 ASCII 草案，可优先转成渲染帧
- 如果下一位继续接地图，先在 `testcpp1` 里验证 `module_05` / `boss_room_01` 的平台可达性、怪物密度和 Boss 前通道节奏，再决定是否继续做门锁和 Boss 正式触发

**最后更新**

- 日期：2026-04-21
- 填写人：Copilot CLI（GPT-5.4）

---

### Copilot：`Copilot CLI（GPT-5.4）`

**Resume**

- `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e`

**负责模块**

- PlayerSandbox 训练场
- EnemySandbox 验证场
- MapDrawer 终端渲染
- 玩家移动 / 跳跃参数微调
- 灵魂 / 法术 / HUD 参数同步
- 主线 `Player.cpp` 战斗与表现接入
- `module_01` / `module_03` 地图细化

**工作目标**

- 把玩家沙盒做成可独立测试战斗手感的稳定环境
- 让玩家与敌人沙盒都能稳定渲染，不叠字、不闪屏
- 把 `docs/parameter_tuning.md` 中的关键参数同步到实现
- 把已经满意的主角攻击 / 法术 / HUD 版本合并到主线

**当前状态**

- 已完成

**已完成**

- 建立并扩展 PlayerSandbox，加入基础攻击、法术、回血、自伤、HUD、无限灵魂等测试能力
- 在 PlayerSandbox 中接入地面敌人刷怪测试，支持 `1 / 2 / C`
- 调整玩家水平移动速度与跳跃高度
- 同步灵魂上限、攻击回魂、法术消耗、回血消耗等参数
- 修复 `HP Debug 5/5 | Soul Debug 0/999` 显示异常
- 修复 player_sandbox 与 enemy_sandbox 的终端渲染问题，改为固定画布覆盖重绘
- 完成向上攻击波、左右攻击波、向下黑砸、基础挥砍的 ASCII 动画第一版
- 把主角当前攻击、法术、HUD、回血与受伤反馈接入主线 `Player.cpp`
- 把 `YOU DEAD` 白屏扩张死亡演出接入主线 `Player.cpp`，并让主线在死亡演出后自动复位
- 按新方案重映射玩家输入：`A/D` 移动，`W/S` 方向修饰，`J` 物理攻击，`K` 法术攻击，组合触发上劈 / 下劈 / 上法术 / 下砸
- 细化 `module_01` / `module_03`：补足可跳平台层次、增加适中偏多的地面怪 / 飞行怪，并保留主线门洞与跑图空间
- 更新 `src/core/main.cpp`、`run.bat`、`docs/module_checklist.md` 以匹配新的主线玩家逻辑
- 将灵魂 HUD 重构为小魂球容器，并修正顶部字符显示问题

**进行中**

- 无

**待处理**

- 玩家状态机
- 受伤击退
- 更完整的主线敌人 / 玩家战斗联动
- 继续配合主线跑图验证 `module_01` / `module_03` 的房间节奏与节点可读性

**阻塞 / 风险**

- Windows 终端行为在不同宿主下可能不完全一致，后续若更换终端环境，渲染层仍需留意兼容性

**需要同步的参数 / 规则**

- 角色移动、跳跃、灵魂、法术相关参数以 `docs/parameter_tuning.md` 为准
- 玩家当前输入映射为：`A/D` 左右移动，`W/S` 上看下看，`J` 物理攻击，`K` 法术攻击，`W/S + J/K` 为方向组合输入
- `module_01` / `module_03` 当前要求：平台必须可跳达、怪物数量偏多但留有移动空间，整体结构向《空洞骑士》式复杂白盒靠拢
- 模块完成进度同步更新到 `docs/module_checklist.md`
- 交互验证优先在 sandbox 中完成，再接主流程

**交接说明**

- 如果下一位 Copilot 要继续做战斗系统，优先从 `src\\player\\Player.cpp`、`src\\core\\main.cpp`、`src\\enemy\\Enemy.cpp` 开始
- 玩家沙盒仍然是表现与手感调试的快速验证场，对照文件是 `src\\sandbox\\PlayerSandbox.cpp`
- 渲染相关问题优先检查 `src\\world\\MapDrawer.cpp`
- 当前 player / enemy sandbox 与主线都依赖共享的 `MapDrawer`
- 如果下一位继续扩图，优先从 `data\\maps\\module_01.map`、`data\\maps\\module_03.map` 和 `docs\\world_layout_foundation.md` 开始

**最后更新**

- 日期：2026-04-21
- 填写人：Copilot CLI（GPT-5.4）
