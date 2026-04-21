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
| Copilot CLI（GPT-5.4） | `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | 项目结构、构建入口、沙盒、地面敌人、飞行敌人原型、Boss 原型 / `CombatSystem`、参数文档、`module_02 / module_04` 地图细化、Soul HUD 修复 | 已完成（本轮交付） | 已完成 Boss demo 下沉到 `Enemy.*`、`CombatSystem` 最小结算接口与相关文档：`boss-demo-into-enemy`、`combat-resolver-implement`、`combat-resolver-docs` 均已 done；主流程位现可直接读取 Boss 状态、攻击信号与一次性击杀奖励 | 仅剩 `combat-resolver-design` 处于 blocked，且该项原本就依赖别的 AI 先完成 `PlayerSandbox` 相关改动，不属于本轮未收尾项 | 下一位接手主流程接线、Boss 正式接入或继续补完整 Battle loop 的 Copilot |
| Copilot CLI（GPT-5.4） | copilot --resume=b8996a16-a959-4def-8a5b-bd2ebdd8eccd | 技能/攻击 ASCII 表现设计、法术分镜、显示语义约定、Player dash 动画、Linux/macOS 主线适配、README 下载/运行说明、终端友好输入重构分工 | 进行中 | 已补充左右冲击波、下砸、普通攻击缺口、敌人受伤/死亡、HKD/HUD 与 `YOU DEAD` 白屏死亡演出的 ASCII 方案；已补主线 `SHIFT` dash 简易拖尾；已完成 Linux/macOS 路径与文档适配；当前按用户最新要求，正在把“多键组合改单键、固定跳高、移动提速、暂停确认界面、键位提示同步”拆成 3 条不抢文件的并行任务 | 无 | 下方 3 条终端友好输入重构任务由其他 AI 认领；本位继续做主线收口与交接 |
| Copilot CLI（GPT-5.4） | `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e` | 玩家沙盒、敌人沙盒、MapDrawer 渲染、参数同步、主线 Player 接入、`module_01 / module_03` 地图细化、主菜单 / 存档最小闭环、捷径门持久化解锁、二段跳 / 普通 dash、Boss 房主流程接线 / 入场对话 / 胜利结算 | 已完成 | 已完成玩家攻击/法术/HUD/`YOU DEAD` 死亡演出接入主线 `Player.cpp`，改成 `W/S` + `J/K` 组合键映射，并细化 `module_01`、`module_03`；已补完 `NEW GAME` / `CONTINUE` / 最小存档闭环与捷径门持久化解锁；现已把普通横向 dash 定为默认 `SHIFT`，并将二段跳改成商人售卖、购买后随存档持久化的能力；本轮已把 `boss_room_01` 接成“进房对白 -> 开战 -> Boss 死亡 -> VICTORY 结算”的主流程闭环，并保持死亡恢复链路可复用 | 无 | 下一个负责玩家状态机、击退、主线战斗、固定存档点、Boss 收尾 polish 或整线跑图验证的人 |
| Copilot CLI（GPT-5.4） | ` copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb` | 参数整理、调参文档、员工文档、进度盘点、地图仓库、`NpcSystem` 最小交互、主界面 HUD 精简、`boss_room_01` 决战房收口、战斗手感回归验证、终端友好输入任务 C | 进行中 | 已完成基础参数归档、员工登记表建立与 resume 对应、8 张白盒地图 + `testcpp1` 主线接线、地图分工表；已细化 `spawn_village` / `random_event_01`，修复 `module_02` 入口卡墙与 `module_01` 边界破损；已落地 `NpcSystem` 最小交互模块，并把主界面改成顶部极简 HUD + 底部常驻操作说明，现已把 `NpcSystem` 正式接进 `main.cpp` 运行时，支持 `E` 交互 / `W/S` 选项 / `J` 购买，并补齐 `Makefile` / `run.bat` 构建项；本轮按最新地图重构任务重做了 `module_05`，并已把 `boss_room_01` 收成右侧入场缓冲 + 中左部开阔主战场的决战房骨架，保留门位、不放杂兵，为 Boss 对话和胜利结算预留空间；此前夜间回归已确认上下法术范围放大和上/下劈砍 `Soul +11` 已落地、只剩地面敌人贴地未收口；本轮已认领并完成“终端友好输入重构”任务 C：`PlayerSandbox` 帮助文本切到 `J/K + U/N/I/M`，并把固定跳高 / 跑速 / 普攻距离 / 起步加速度的当前回归结论写回调参文档 | 仍待任务 A / B 把主线 `Player.*`、`main.cpp`、`README.md` 完整收口后，再做一次整线复核并关闭终端友好输入插单 | 下一位接手 `Enemy.*` 剩余贴地修复，或在 A / B 合并后继续补最终整线验证记录的 Copilot |

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
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | 紧急任务 3：Soul 单个容器重绘修复 | 已完成 | 只在 `src\\player\\Player.cpp` 内做最小修复：Soul 单格填充改成稳定左对齐，读数改成固定宽度，避免单格 / 数值变化时出现错位、覆盖或残影感 | 下一位接手继续做 Boss 主流程接线或整线跑图验证的 Copilot | 未改 `main.cpp`、敌人和 NPC 模块；临时编译验证通过 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | Boss demo 视觉语言统一 | 已完成 | 按统一字符语言重做 `BossSandbox`：近战 Boss 现将 `!` 用作危险核心、`=` 用作冲刺路径、`/ \\` 用作挥砍、`^` 用作落点预警、`*` 用作命中爆点，并给 JumpSlash / DashSlash / SweepSlash 分开了可读前摇与短余波 | 下一位接手继续补远程 Boss 分镜、Boss 房正式接线或整线跑图验证的 Copilot | 保持“前摇清楚 > 命中强烈 > 余波很短”，本轮未改 `main.cpp` |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | 新任务 B：BossSandbox Boss demo | 已完成 | 把 `BossSandbox` 从框架推进到可打 demo：近战 Boss 现可苏醒、追位、受击、硬直、死亡，并具备横扫、前冲斩、跳劈三种基础招式；仍保留远程原型用于后续扩展 | 下一位接手主流程接线、Boss 房正式接入或继续补 Boss 细节的 Copilot | 本轮仍未触碰 `main.cpp` / 存档 / NPC 文件 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | Boss demo 下沉到 `Enemy.*` | 已完成 | 把近战 Boss 的启动预警、横扫 / 冲刺斩 / 跳劈命中图样、短余波和落点位移规格写回 `include\\enemy\\Enemy.h` / `src\\enemy\\Enemy.cpp`；`BossSandbox` 现只负责消费这些规格并显示 | 下一位接手继续补远程 Boss 视觉规格、Boss 房接线或主流程正式刷入的 Copilot | 这次仍未触碰 `main.cpp`，但 Boss 读招规则已不再只存在于沙盒里 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | `module_02` / `module_04` 最新重构任务 | 已完成 | 按最新地图重构标准重做两张图：改成分段推进、分层清敌和蛇形穿越；保留原门位不动，重新安放地面怪并校正行宽、自查门位与刷怪支撑 | 下一位接手主程序跑图验证或继续补 Boss 主流程接线的 Copilot | 本轮只改 `data\\maps\\module_02.map`、`data\\maps\\module_04.map` 与 staff 同步记录 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e` | 并行任务 A：主菜单 / 存档最小闭环 | 已完成 | 接通 `GameSession` / `SaveSystem` / `main.cpp`，让标题页按真实存档显示 `CONTINUE`，新游戏会落盘，切图自动存档，死亡后按当前存档点恢复 HP / Soul / HKD / 地图与出生点，并补齐 `Makefile` / `run.bat` 构建项 | 下一位接手固定存档点、难度选择或更完整的流程 UI 的 Copilot | 已用最小存档烟测确认存读与死亡恢复链路可用 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e` | 紧急任务 1：捷径门持久化解锁 | 已完成 | 把 `module_03 <-> spawn_village`、`module_05 <-> spawn_village` 改成村侧初始锁定、远侧交互解锁、解锁后双向通行；补齐 `module_05` 缺失的村侧入口出生点，并把解锁状态写进存档 | 下一位接手固定存档点、暂停菜单或整线跑图验证的 Copilot | 已通过门目标出生点校验和存档持久化烟测；当前主程序编译验证需带上 `src\\combat\\CombatSystem.cpp` |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e` | 新任务 A：二段跳 + 普通 Dash | 已完成 | 在 `Player` / `PlayerSandbox` 中加入 `SPACE` 二段跳和 `SHIFT` 普通横向 dash；随后把主流程改成初始仅开放 dash、二段跳需通过商人购买 `Double Jump Crest` 才能解锁，且会写入存档并更新底部操作提示 | 下一位接手玩家状态机、正式能力解锁扩展或主流程教学提示的 Copilot | 已完成 `player_sandbox.exe` / `testcpp1.exe` 编译，并用临时 harness 验证二段跳解锁前后差异、商店购买和存档持久化都生效 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | 新任务 C：单向门 D 标记位置修正 | 已完成 | 只改 `data\\maps\\spawn_village.map`，把通往 `module_05` 的村侧捷径门移到左边界，把通往 `module_03` 的村侧捷径门移到下层地面门位，同时把对应回村出生点贴到门旁，解决出生村里两扇单向门悬在房间内部的问题 | 下一位接手整线跑图验证或继续微调村庄门位可读性的 Copilot | 本轮按用户要求只修出生点图里的两扇门；相关落点已重新校验通过 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | 主线地图重构任务下发 | 已完成 | 按“房间必须有完整穿越路径、不能从 A 点直跑到 B 点”的新标准，重排 6 张主线非特殊房的分工：排除 `random_event_01` 与 `boss_room_01`，其余 6 张图分配给另外 3 个 AI 并要求严格分开开发 | 各房间负责人按新标准分别重构 | 本轮核心要求是补纵向墙体、切断直线通路、让每个平台和每堵墙都有明确设计目的 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | `module_01` 平台链重做 | 已完成 | 在不移动左右两个门位的前提下，重做 `data\\maps\\module_01.map` 的主要地形：把原先大量无效平台改成连续可用的爬升 / 下落 / 再起跳路线，并补上多段阻挡墙，让玩家推进时必须真正利用平台链 | 下一位接手整线跑图验证或继续微调 `module_01` 手感的 Copilot | 本轮只改平台和墙体，不改门位；行宽与 transition 坐标已重新检查 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | `module_05` 长路径重构 | 已完成 | 按最新地图重构标准只重做 `data\\maps\\module_05.map`：保留 Boss 门、村庄捷径门和模块 4 入口门位不动，改成必须先从 `module_04` 入口爬上中层主桥，再分支前往左侧 Boss 门或右侧捷径门的长路线，并把 3 个陆地敌人压到真实路线节点上 | 下一位接手整线跑图验证或继续微调 Boss 前压迫感的 Copilot | 用户已明确要求本轮不要再动 `spawn_village`，所以出生村保持现状；`module_05` 的门位、刷怪落点和行宽已重新自查 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | Boss 房成型并行任务下发 | 已完成 | 按“Boss 房重做 + Boss 正式嵌入 + 入场对话 + 死亡后胜利结算”这一条主线，把工作拆给 3 个 AI：地图、Boss 本体/战斗、主流程接线/UI 各自独立推进，确保可同时开发且不抢文件 | 3 位负责人各自认领并行推进 | 当前目标是把 demo 从“很好玩的原型”收成“有终点、有结算的完整成型版” |
| 2026-04-21 | Copilot CLI（GPT-5.4） | `boss_room_01` 决战房收口 | 已完成 | 只改 `data\\maps\\boss_room_01.map`：去掉杂兵刷点，保留右侧回程门位和出生点不动，改成右侧短入场缓冲 + 中左部开阔横向战斗区，并只留下少量高位平台与右侧顶棚，给 Boss 登场对话与胜利结算预留清晰空间 | 下一位接手 Boss 房主流程接线、Boss 刷入验证或整线跑图验证的 Copilot | 已自查行宽、门位、出生点脚下支撑和无封死路线；当前不负责 Boss 刷入逻辑与对话流程 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | Boss demo 下沉 / `CombatSystem` 最小结算接口 | 已完成 | 该线负责人已确认：`boss-demo-into-enemy`、`combat-resolver-implement`、`combat-resolver-docs` 全部 done；Boss 状态读取、攻击信号消费和一次性击杀奖励接口已可供主流程位直接接入 | 下一位接手 Boss 房主流程接线 / `VICTORY` 结算的 Copilot | 仅剩 `combat-resolver-design` 为 blocked，但它本来就依赖其他 AI 先完成 `PlayerSandbox` 相关改动，不属于这次交付未收尾 |
| 2026-04-21 | Copilot CLI（GPT-5.4） | 战斗手感优化插单下发 | 已完成 | 记录用户实机发现的 3 个问题：`g` 地面敌人冲刺后可能浮空、上下劈砍未增加 Soul、上下法术判定范围偏小；并按不抢文件的原则拆给 3 个 AI 分开优化 | 3 位负责人各自认领并行推进 | 本轮要求优先做最小可感知优化，不顺手扩大成全面重构 |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb` | 战斗手感插单回归验证 / 文档回填 | 已完成 | 只读当前仓库代码并重新编译 `player_sandbox.exe`、`enemy_sandbox.exe`、`testcpp1_validation.exe` 复核：向上法术现为 5 格纵向列 + 3 格 crown，向下法术现为整段下砸列 + 3 格落地爆点，判定范围已较旧版放大；随后按用户提示复查到 `Player.cpp` 已于更晚版本把上劈 / 下劈命中后的 `Soul +11` 补齐；当前仍只剩 `GroundEnemy::DashAttack / AttackRecovery` 路径未统一贴回 `spawnPosition.y` | 下一位接手 `Enemy.*` 剩余贴地修复，再由文档位复核收口 | 本轮严格未越界改代码；结论已同步回 `parameter_tuning.md` 与 staff |
| 2026-04-21 | Copilot CLI（GPT-5.4） `copilot --resume=b8996a16-a959-4def-8a5b-bd2ebdd8eccd` | Player dash 简易动画 | 已完成 | 在 `Player` 主线渲染里给 `SHIFT` dash 补了简易拖尾和前缘方向提示：冲刺前半程更亮、后半程衰减，不改 dash 判定和位移逻辑，只增强可读性 | 下一位接手继续做 dash 手感 polish 或整线跑图验证的 Copilot | 本轮只改 `Player.*` 的显示层，不触碰主流程和战斗结算 |
| 2026-04-22 | Copilot CLI（GPT-5.4） `copilot --resume=b8996a16-a959-4def-8a5b-bd2ebdd8eccd` | Linux/macOS 主线适配 / README 重写 | 进行中 | 正在把主线运行时输入层与提示同步到 Linux/macOS：保留 Windows `SHIFT` dash，同时将非 Windows dash 改为 `Q`；README 需改成清晰写明 Linux / macOS 的下载、编译与运行步骤，并按要求移除一键安装包说明 | 下方 3 个并行任务由其他 AI 认领；我继续负责主线代码与 README | 当前主改动面集中在 `KeyStateManager.*`、主线提示文本和 `README.md` |
| 2026-04-22 | Copilot CLI（GPT-5.4） `copilot --resume=b8996a16-a959-4def-8a5b-bd2ebdd8eccd` | 终端友好输入重构分工 | 已完成 | 按用户新要求把终端友好控制重构拆成 3 条并行线：1）`Player.*` / `KeyStateManager.*` 负责单键改绑、固定跳高、跑速 +25%、取消启动加速度、普攻距离 +1；2）`main.cpp` / `README.md` 负责暂停确认界面、`ESC` 退出改键、底部提示与运行说明；3）`PlayerSandbox.cpp` / `docs` 负责沙盒帮助文本、参数文档和最终回归记录 | 3 位负责人分别认领 | 本轮先做文件级分工，避免多名 AI 同时改 `Player.*` / `main.cpp` / 文档造成冲突 |
| 2026-04-22 | Copilot CLI（GPT-5.4） `copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb` | 终端友好输入重构任务 C：沙盒提示 / 文档 / 回归记录 | 已完成（本轮职责） | 已把 `PlayerSandbox` 的帮助文本与方向技输入改成 `J/K + U/N/I/M`；同时把固定跳高、跑速、普攻距离、取消起步加速度的当前仓库状态写回 `docs\\parameter_tuning.md`，明确区分“沙盒提示已同步”和“主线 `Player.*` 仍待任务 A 收口” | 下一位接手任务 A / B 合并后的主线整体验收，或继续补最终整线验证记录的 Copilot | 本轮未越界修改 `Player.*`、`KeyStateManager.*`、`main.cpp` 或 `README.md`；地面敌人贴地问题仍是独立待修项 |

---

## Linux / macOS 适配并行任务下发

这轮我的主任务是：**把主线 `hongkongknight` 适配到 Linux / macOS，并把 README 改成三端清晰下载 / 运行说明**。

为了不抢文件，另外三项任务拆给 3 个 AI：

1. **并行任务 A：沙盒提示与键位同步**
   - 文件范围：`src\sandbox\PlayerSandbox.cpp`、`src\sandbox\EnemySandbox.cpp`、`src\sandbox\BossSandbox.cpp`、`src\sandbox\WorldSandbox.cpp`
   - 目标：把所有沙盒里的操作提示、底部 HUD、帮助文本同步成跨平台版本；Windows 继续写 `SHIFT` dash，Linux/macOS 写 `Q` dash
   - 不要碰：`src\core\main.cpp`、`src\input\KeyStateManager.cpp`、`README.md`

2. **并行任务 B：跨平台文档补量**
   - 文件范围：`docs\game_design.md`、`docs\parameter_tuning.md`、`docs\copilot_staff.md`
   - 目标：把跨平台控制说明、主程序名 `hongkongknight`、Linux/macOS 运行假设同步进设计文档与参数文档
   - 不要碰：主线代码、README、沙盒代码

3. **并行任务 C：Unix 烟测与问题登记**
   - 文件范围：只允许更新 `docs\copilot_staff.md`
   - 目标：在 Linux / macOS 环境实际烟测 `make`、`make run`、标题页输入、主线移动 / 跳跃 / `Q` dash / 交互是否可用；如果有问题，只做登记，不直接改代码
   - 不要碰：任何 `src\`、`include\`、`README.md`

---

## 终端友好输入重构并行任务下发

用户最新要求已经明确：**把吃同时按键的操作收口成终端友好版本**，优先减少 Linux terminal / SSH 场景下的多键输入依赖。

本轮目标清单：

1. 把需要同时按两个键的主线操作改成单键：
   - 上劈 -> `U`
   - 下劈 -> `N`
   - 上法术 -> `I`
   - 下法术 -> `M`
2. `SPACE` 改成固定高度起跳，起跳后不再受后续长按 / 松开影响
3. 普攻攻击距离 `+1`
4. 跑速提升 `25%`
5. 取消起步加速度，按下左右立即用目标速度移动
6. 城镇里原本和其他 `L` 混淆的老人字母改成 `O`
7. 所有键位帮助文本 / HUD / README 统一改成新映射
8. 不再直接用 `ESC` 立刻退出；改成暂停确认小界面，再确认退出

为了不抢文件，这轮拆给 3 个 AI：

1. **并行任务 A：玩家控制 / 输入手感重构**
   - 文件范围：`include\input\KeyStateManager.h`、`src\input\KeyStateManager.cpp`、`include\player\Player.h`、`src\player\Player.cpp`
   - 负责内容：
     - 上劈 / 下劈 / 上法术 / 下法术改单键 `U / N / I / M`
     - `SPACE` 固定高度跳跃
     - 跑速 +25%
     - 取消左右移动起步加速度
     - 普攻攻击距离 +1
   - 不要碰：`src\core\main.cpp`、`README.md`、`docs\*`

2. **并行任务 B：主线 UI / 提示 / 暂停确认界面**
   - 文件范围：`src\core\main.cpp`、`README.md`
   - 负责内容：
     - 底部操作提示、标题页提示、主线帮助文案统一改成新键位
     - `ESC` 不再直接退出，改成暂停 / 是否退出确认小界面
     - 主线中城镇老人显示字母从 `L` 改成 `O`
     - README 的 Quick Start / Controls 同步改成新键位
   - 不要碰：`Player.*`、`KeyStateManager.*`、`docs\parameter_tuning.md`

3. **并行任务 C：沙盒提示 / 文档 / 回归记录**
   - 文件范围：`src\sandbox\PlayerSandbox.cpp`、`docs\parameter_tuning.md`、`docs\copilot_staff.md`
   - 负责内容：
     - `PlayerSandbox` 的帮助文本、HUD 提示改成新键位
     - 参数文档同步记录：固定跳高、跑速 +25%、普攻距离 +1、取消起步加速度
     - 实施后在 staff 里登记“终端友好输入重构”的回归结果
   - 不要碰：`src\core\main.cpp`、`Player.*`、`KeyStateManager.*`

---

## 当前主任务

现在的主任务已经收口成一条非常明确的线：**把 `boss_room_01` 做成 demo 终点**。

当前优先级：

1. 重做 `boss_room_01`：地图不再是普通白盒，而是带决战感、可读、可打的 Boss 房
2. 把 Boss 正式接进主流程：进房触发登场对话，结束对话后开战
3. 接通 Boss 死亡后的胜利结算，让 demo 有明确终点和收束
4. 上述主线闭环完成后，再考虑 `random_event_01`、剧情补量和更多收尾内容

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

## Boss 房成型并行任务下发（3 个 AI，必须同时开发且不能冲突）

当前目标：**把 `boss_room_01` 收成 demo 的终点房。**  
闭环要求只有一条：

1. 玩家进入 `boss_room_01`
2. 先触发 Boss 登场对话
3. 对话结束后正式开战
4. 玩家死亡可按现有主流程正常恢复
5. Boss 死亡后显示胜利结算
6. 让当前 demo 在 Boss 房这里形成清晰的完成感

并行规则：

1. `main.cpp` / 主流程接线只允许 **1 个 AI** 改。
2. `Enemy.*` / `CombatSystem.*` 只允许 **1 个 AI** 改。
3. `boss_room_01.map` 只允许 **1 个 AI** 改。
4. 如果需要别的模块配合，先在 staff 写“需要预留 hook / 读取某状态”，不要跨线直接改。
5. 这轮优先做最小可交付版本，不额外扩随机事件、不扩大剧情树。

### 并行任务 A：Boss 房主流程接线 / 入场对话 / 胜利结算

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e`

**目标**

- 让 `boss_room_01` 在主程序里真正形成“进房 -> 对话 -> 开战 -> 胜利结算”的流程
- 复用现有死亡恢复 / 存档闭环，不另起新体系
- 让 Boss 战后有明确的胜利落点，而不是直接回到普通跑图态

**只负责这些文件**

- `src\core\main.cpp`
- `include\core\GameSession.h`
- `src\core\GameSession.cpp`
- 如确实需要再改：`include\save\SaveSystem.h`
- 如确实需要再改：`src\save\SaveSystem.cpp`

**本轮交付**

- 玩家首次进入 `boss_room_01` 会先进入 Boss 开场状态
- 屏幕上显示 1~3 句 Boss 入场台词，玩家可按现有确认键推进
- 对话结束后正式刷 Boss / 激活 Boss 战逻辑
- Boss 死亡后显示最小胜利结算：`VICTORY`、击败 Boss、奖励 / 收尾文案
- 若玩家在 Boss 战中死亡，仍按当前主流程稳定恢复，不把流程卡死

**禁止触碰**

- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `include\combat\CombatSystem.h`
- `src\combat\CombatSystem.cpp`
- `data\maps\boss_room_01.map`

### 并行任务 B：Boss 本体 / 攻击节奏 / 战斗结算

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af`

**目标**

- 把当前 Boss demo 从 sandbox 级原型推进成可供主流程刷入的正式 Boss 模块
- 保证 Boss 至少有完整的出招、受击、硬直、死亡和奖励发放
- 不碰主流程接线，只把接口和状态做扎实

**只负责这些文件**

- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `include\combat\CombatSystem.h`
- `src\combat\CombatSystem.cpp`
- 如确实需要验证再改：`src\sandbox\BossSandbox.cpp`

**本轮交付**

- 至少 1 个正式可刷入的 Boss 形态
- Boss 至少具备：待机 / 追位、2~3 个主力攻击、受击、硬直、死亡
- Boss 死亡后能给出统一的击杀 / 奖励结果，供主流程位直接消费
- 若需要主流程位配合，只能在 staff 中写清“需要读取的 Boss 状态 / 回调点”

**主流程位需要读取的 Boss 状态 / 回调点**

- `Boss::getState()`：用于区分 `Dormant -> Intro -> Positioning / AttackStartup / AttackRecovery / Staggered -> Dead`
- `Boss::hasEncounterStarted()`：用于判断玩家是否已经唤醒 Boss、是否该结束入场对白阶段
- `Boss::isCombatActive()`：用于判断 Boss 是否已经进入正式战斗态
- `Boss::consumeAttackSignal(...)`：用于读取本轮出招信号并驱动命中 / 特效 / 主流程侧攻击表现
- `Boss::consumeDefeatReward(...)`：用于在 Boss 死亡后只消费一次统一奖励结果，避免主流程重复发奖励

**禁止触碰**

- `src\core\main.cpp`
- `include\core\GameSession.h`
- `src\core\GameSession.cpp`
- `data\maps\boss_room_01.map`
- `include\save\SaveSystem.h`
- `src\save\SaveSystem.cpp`

### 并行任务 C：`boss_room_01` 地图重做 / 决战房结构收口

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb`

**目标**

- 把 `boss_room_01` 从普通白盒战斗房改成真正的决战房
- 保证 Boss 有足够读招与位移空间，同时保留玩家躲避和恢复节奏
- 给 Boss 登场对话和胜利结算预留清楚的入场区 / 战斗区 / 收尾感

**只负责这些文件**

- `data\maps\boss_room_01.map`
- 如需同步文字说明：`docs\game_design.md`
- 如需同步参数：`docs\parameter_tuning.md`

**本轮交付**

- `boss_room_01` 保留正确门位，不乱动 transition 位置
- 入口进入后要先有短缓冲区，再进入主要战斗区
- 房间整体以横向 Boss 战为主，平台少而精，不能破坏读招
- 不放杂兵，不把 Boss 房再做成普通清怪房
- 交货前自查：门位正确、路线不封死、Boss 战核心活动区域足够清晰

**禁止触碰**

- `src\core\main.cpp`
- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `include\combat\CombatSystem.h`
- `src\combat\CombatSystem.cpp`

说明：

- 任务 A 是唯一允许改主流程的人。
- 任务 B 先把 Boss 模块和结果接口做稳，等待任务 A 接入。
- 任务 C 只管房间结构，不管 Boss 刷入逻辑和对话流程。
- 这 3 条线都完成后，当前 demo 就可视为进入“成型可交付”状态。

---

## 战斗手感优化插单（3 个 AI，继续分开开发）

用户实机反馈的 3 个问题：

1. `g` 地面敌人冲刺后有概率浮空，疑似缺少冲刺结束后的贴地 / 落地处理
2. 上下劈砍目前没有增加 Soul
3. 向上 / 向下法术的判定范围偏小，需要适度放大

拆分原则：

1. `Enemy.*` / `CombatSystem.*` 仍然只允许 **1 个 AI** 改
2. `Player.*` 仍然只允许 **1 个 AI** 改
3. 验证 / 文档 / 参数同步由另一个 AI 负责，不反向覆盖代码线
4. 本轮只修用户已经明确指出的问题，不顺手做大规模战斗重构

### 插单 A：地面敌人冲刺后贴地修正

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af`

**目标**

- 修正 `g` 地面敌人冲刺结束后偶发浮空的问题
- 保证冲刺后的敌人仍然落在真实平面上，不悬空、不停在错误高度

**只负责这些文件**

- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- 如确实需要最小配合：`include\combat\CombatSystem.h`
- 如确实需要最小配合：`src\combat\CombatSystem.cpp`

**本轮交付**

- 冲刺结束后的地面敌人不会停在半空
- 不破坏现有索敌、前摇、攻击触发和门前空间
- 如需要贴地补偿逻辑，应写在敌人 / 战斗侧，不回退到地图侧乱修

**禁止触碰**

- `src\player\Player.cpp`
- `include\player\Player.h`
- `src\core\main.cpp`

### 插单 B：玩家上下劈砍 Soul 与上下法术判定范围优化

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e`

**目标**

- 让上劈 / 下劈命中后也能按合理规则增加 Soul
- 适度扩大向上 / 向下法术判定范围，让手感不再过窄

**只负责这些文件**

- `include\player\Player.h`
- `src\player\Player.cpp`
- 如需同步参数：`docs\parameter_tuning.md`

**本轮交付**

- 上劈 / 下劈命中后 Soul 增长规则与横斩逻辑一致或保持明确统一
- 向上法术和向下法术判定范围明显比当前更宽，但不要夸张到穿透半张图
- 不破坏现有 `J/K`、`W/S` 输入映射和 Boss 房主流程

**禁止触碰**

- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `src\core\main.cpp`
- `include\combat\CombatSystem.h`
- `src\combat\CombatSystem.cpp`

### 插单 C：实机回归验证 / 参数回填 / staff 同步

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb`

**目标**

- 在前两条线完成后，专门验证这 3 个问题是否真正消失
- 补齐文档和员工登记，不让这轮优化只停留在口头说明

**只负责这些文件**

- `docs\parameter_tuning.md`
- `docs\game_design.md`
- `docs\copilot_staff.md`

**本轮交付**

- 明确记录：地面敌人冲刺贴地、上下劈砍 Soul、上下法术判定范围 3 项都已回归验证
- 若发现新问题，只在 staff 里记录，不直接越界改代码文件
- 把本轮战斗手感优化的最终结论写回文档

**禁止触碰**

- `src\player\Player.cpp`
- `include\player\Player.h`
- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `src\core\main.cpp`
- `include\combat\CombatSystem.h`
- `src\combat\CombatSystem.cpp`

**回归结论（当前仓库夜间复核）**

- 向上 / 向下法术判定范围：**已通过**
  - 当前实现已是更宽的纵向列 + 终段扩展爆点，不再是过窄的单线命中感。
- `g` 地面敌人冲刺后贴地：**未通过**
  - `GroundEnemy::DashAttack / AttackRecovery` 路径仍未统一贴回地面高度，浮空风险未算真正关闭。
- 上劈 / 下劈命中加 Soul：**已通过**
  - `Player.cpp` 当前已在上劈 / 下劈命中分支同步执行 `Soul +11`，与横斩收益规则保持一致。

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

## 下一阶段并行任务下发（以当前主链为准）

假设当前主线、存档、NPC 最小交互都已稳定，下一阶段的重点不再继续铺大系统，而是补**移动能力、Boss demo、门位修正**这 3 条线。  
要求：仍然**分开开发、并行推进、各自只动自己的文件组**。

### 新任务 A：二段跳 + 普通 Dash（先在沙盒验证）

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e`

**目标**

- 做出《空洞骑士》风格的最小版二段跳
- 做出普通横向 dash（先不做黑冲，不做无伤穿敌）
- 优先在沙盒里把手感和可用性跑通，再考虑正式接主流程

**只负责这些文件**

- `include\player\Player.h`
- `src\player\Player.cpp`
- `src\sandbox\PlayerSandbox.cpp`
- 如需文档同步：`docs\parameter_tuning.md`

**本轮交付**

- 二段跳可独立触发
- 普通 dash 可左右触发
- 至少在 `PlayerSandbox` 中完成可验证演示
- 不要求这一轮直接写进商店购买或主流程永久解锁

**禁止触碰**

- `src\core\main.cpp`
- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `data\maps\*.map`

### 新任务 B：BossSandbox 内搓出一个 Boss demo

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af`

**目标**

- 按核心文档先做出一个能打、能受击、能验证节奏的 Boss demo
- 先在 `BossSandbox` 里跑，不要求这一轮就正式接进主流程
- 优先验证 Boss 的体量、攻击节奏和基础战斗读感

**只负责这些文件**

- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`
- `src\sandbox\BossSandbox.cpp`
- 如确实需要：`include\combat\CombatSystem.h`、`src\combat\CombatSystem.cpp`
- 文档对照：`docs\game_design.md`

**本轮交付**

- 至少 1 个 Boss demo
- Boss 至少具备：血量、受击、死亡、1~2 个攻击、基础行为切换
- 能在 `BossSandbox` 里独立验证
- 不要求这一轮就补完整 Boss 房流程和通关逻辑

**禁止触碰**

- `src\core\main.cpp`
- `include\save\SaveSystem.h`
- `src\save\SaveSystem.cpp`
- `include\npc\NpcSystem.h`
- `src\npc\NpcSystem.cpp`

### 新任务 C：单向门 D 标记位置修正

**负责人**

- Copilot CLI（GPT-5.4） `copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb`

**目标**

- 修正当前单向门 `D` 标记和实际地图接壤位置的偏差
- 保证上下相邻的箱庭，门一定出现在两张图真正接壤的地面/边界处
- 优先修正视觉与交互的一致性，不扩展成大规模门系统重构

**只负责这些文件**

- `data\maps\spawn_village.map`
- `data\maps\module_03.map`
- `data\maps\module_05.map`
- 如确实需要：`docs\world_layout_foundation.md`

**本轮交付**

- `D` 标记位置和实际门位一致
- 上下相邻房间的门在视觉上真正接壤
- 整线跑图时，门提示不再出现“明明在上下关系却悬在奇怪位置”的问题

**禁止触碰**

- `src\core\main.cpp`
- `include\player\Player.h`
- `src\player\Player.cpp`
- `include\enemy\Enemy.h`
- `src\enemy\Enemy.cpp`

---

## 当前地图重构分工（3 个 AI / 6 张主线非特殊房）

这一轮**不处理** `random_event_01` 和 `boss_room_01`。  
只重构：

- `spawn_village`
- `module_01`
- `module_02`
- `module_03`
- `module_04`
- `module_05`

统一重构标准：

1. 禁止从 A 点到 B 点出现“大平地直跑通关”。
2. 必须补出纵向墙体、落差和绕行，让玩家像在“穿越整个模块”。
3. 每个平台都要有职责：跳跃衔接、战斗落点、转层、控视野或门前缓冲。
4. 每堵墙都要有职责：切断捷径、限制直冲、迫使换层、组织战斗。
5. 敌人要嵌进路线节点，不是随手撒在空地上。
6. **绝不允许把玩家封死**：必须保证入口到出口始终可跑通，不能出现死路、软锁、跳下去回不来却又无法前进的封死结构。
7. 每张图至少要保留一条稳定主路线；即使做分层、绕行和墙体阻挡，也只能“拉长路线”，不能“掐断路线”。
8. 现有单向门、隐藏门必须纳入路线设计：门前后都要有合理到达路径，不能把门做成孤点、摆设或错位捷径。
9. 门的位置视为当前关卡骨架的一部分：**必须位于合适且正确的位置，且本轮不允许随意移动门位**。
10. 交货前必须自查：所有陆地敌人必须落在真实可站立的平面上，不能悬空、卡边或站在不成立的平台端头。
11. 交货前必须自查：所有门都要处在正确接壤位置，位置可读、可到达、不可漂浮，也不能在这轮重构里被擅自挪位。
12. 仍然保持**分开开发**：每个 AI 只改自己那两张图，不抢别人的地图文件。

交货前自查（强制）：

1. 入口到出口至少有一条稳定可跑通路线，不存在死路或软锁。
2. 所有陆地敌人都站在平面上，不出现悬空刷怪或脚下半空。
3. 所有单向门、隐藏门、普通门都接在正确路线节点上，不是摆设。
4. 所有门位保持正确且固定，不因这轮重构被随意移动。

| Copilot / Resume | 分配地图 | 本轮重构重点 | 交付要求 |
| --- | --- | --- | --- |
| Copilot CLI（GPT-5.4） `copilot --resume=afb13241-a3b1-49f4-8863-626279fc7f6e` | `module_01`、`module_03` | 主线前段推进手感、首轮节奏建立、三向节点房拆路径 | `module_01` 必须从“能过去”改成“必须利用整段平台链推进”；`module_03` 必须把主线、捷径门、支线事件口拆成明确分层路线，不能贴地一口气横穿；两张图都必须保证从入口到所有主线出口存在稳定可跑通路线，并确保相关门位不移动、陆地敌人全部落在平面上 |
| Copilot CLI（GPT-5.4） `copilot --resume=fdc64aaa-c115-4144-99c3-7771e98aa2af` | `module_02`、`module_04` | 中段战斗模块化、纵向阻挡、房间内完整穿越路径 | 两张图都要从散平台改成“分段推进 + 分层清敌”；玩家不能从入口顺着底层一路跑到出口，至少要经历一次上抬、一次转层和一次被墙体逼出的绕路；禁止做出跳下后无路返回或无路前进的封死段，并在交货前确认门位正确固定、陆地敌人无悬空 |
| Copilot CLI（GPT-5.4） `copilot --resume=ef54648a-46f6-4977-b24c-6dd9837bb4bb` | `spawn_village`、`module_05` | 基地可读性、捷径门记忆点、Boss 前长路径压迫感 | `spawn_village` 仍保持安全区，但主出口、两扇捷径门和生活区必须分层清楚；`module_05` 必须重构成真正的 Boss 前穿越模块，不能让玩家轻易横跑到出口，要通过墙体、平台链和守点敌人把路线拉长；两张图都必须避免把玩家卡进死角或单向封死区域，并把单向门/隐藏门接进正确路线，保持门位固定 |

说明：

- `random_event_01` 本轮冻结，不纳入这次主线房间重构。
- `boss_room_01` 本轮冻结，继续留给 Boss 战正式接线时再单独处理。
- 我这一位这轮不再占地图名额，只负责下发标准、同步 staff、看整体结构是否收口一致。

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
- 按最新 staff 标准完成 `module_02`、`module_04` 二次重构：两图都改成“分段推进 + 分层清敌 + 墙体逼绕路”，并保持门位固定、陆地敌人全部落在真实平面上
- 完成 Boss 共用血量 / 硬直 / 5 秒硬直窗口框架，并新建 `BossSandbox`
- 完成 `BossSandbox` 近战 Boss demo：横扫、冲刺斩、跳劈、受击、硬直、死亡、奖励结算均可单独验证
- 完成 Boss 视觉语言第一轮统一：`!` / `=` / `/ \` / `^` / `*` / `. # < >` 已用于近战 Boss demo 的前摇、命中和短余波
- 完成近战 Boss 视觉规格下沉：`Enemy.*` 现直接产出启动预警、命中帧、收招帧与位移落点，`BossSandbox` 不再维护第二套近战图样
- 完成紧急任务 3：Soul 单个容器重绘修复，稳定单格填充和数值读数的 HUD 重绘
- 更新 `docs/game_design.md` 与 `docs/parameter_tuning.md`
- 已把阶段性代码推送到 GitHub

**进行中**

- 已认领并行任务 B：Boss 原型 / 敌人扩展 / 战斗结算层最小化
- 已补出 `BossSandbox` 近战 Boss demo，并把这套近战技能规格下沉回 `Enemy.*`；接下来继续细化远程 Boss 分镜、Boss 房接线和主流程接入点
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
- 紧急任务 3 当前已在 `src\player\Player.cpp` 完成：Soul 单格填充固定为稳定位置，Soul 读数固定宽度输出
- 新任务 B 当前已在 `BossSandbox` 落地近战 Boss demo，但正式刷进 `boss_room_01` 仍需主流程位后续接线
- 当前 Boss demo 统一规则是：前摇必须先于判伤，不同招式必须用不同预警语言，命中帧最亮，收招帧极短
- 近战 Boss 当前的横扫 / 冲刺斩 / 跳劈视觉规格已收口到 `Enemy.*`，后续若补主流程或别的 sandbox，应直接复用 Boss 模块接口
- Boss 模块现已额外提供 `hasEncounterStarted()`、`isCombatActive()`、`consumeAttackSignal(...)`、`consumeDefeatReward(...)` 4 个主流程侧 hook

**交接说明**

- 如果下一位要接主流程，请先给 Boss 刷入 `boss_room_01` 和 `CombatSystem` 调用预留 hook，再接正式战斗入口
- 主流程位接 Boss 时，优先按 `getState()` / `hasEncounterStarted()` 切开场对白，再按 `isCombatActive()` 放战斗输入，最后用 `consumeDefeatReward(...)` 接胜利结算
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
- 已修正 `spawn_village` 里两扇单向捷径门的村侧门位：`module_05` 门贴到左边界，`module_03` 门贴到下层地面，不再悬空
- 已重做 `module_01` 的主路径平台链，保留两端门位不动，并用多段墙体把推进节奏收成“上去 -> 横移 -> 下来 -> 再上去 -> 落门口”
- 已重做 `module_03` 的三向节点路线：把主线左出口、上方捷径门和下方事件口拆成明显分层，并用两端底层阻挡切掉贴地横穿

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
- Boss 房主流程接线 / 入场对话 / 胜利结算

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
- 已把 `boss_room_01` 接成正式 demo 终点：首次入房先播 3 句 Boss 入场对白，按 `ENTER` 推进；对白结束后正式激活 Boss 战，期间封住退路；Boss 死亡后显示 `VICTORY`、击败提示与 HKD 奖励；玩家若战败则仍按现有存档 / 死亡恢复链路回到房间入口重来，不会卡死流程

**进行中**

- 无

**待处理**

- 玩家状态机
- 受伤击退
- 更完整的主线敌人 / 玩家战斗联动
- 继续配合主线整线跑图验证（重点看 `boss_room_01` 闭环、各门位、NPC、存档和前后模块衔接）

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
