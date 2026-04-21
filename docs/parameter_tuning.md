# 调参文档

## 来源

- 记录批次：2026-04-21
- 来源：用户口述数值机制（移动、跳跃、灵魂）
- 说明：当前文档只保留基础调参项；单位暂按“游戏内速度/高度/时间单位”理解，后续可再统一换算。
- 范围：仅记录基础移动、基础跳跃、基础灵魂参数，不做游泳系统，不做复杂 HUD/动画参数。

## 1. 移动参数

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| player | 常规移动 | run_speed | 8.3 | 速度 | 慢跑时地面水平移动速度 |
| player | 常规移动 | air_move_speed | 8.3 | 速度 | 常态空中水平移动速度 |
| player | 庄重区域移动 | solemn_move_speed | 6 | 速度 | 特定庄重区域的地面/空中移动速度 |
| player | 地图查看移动 | map_walk_speed | 4 | 速度 | 打开地图并短暂停顿后的移动速度 |

## 2. 跳跃参数

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| player | 跳跃起跳 | jump_initial_upward_speed | 17.11 | 速度 | 按下跳跃键后的初始上升速度；本轮上调约 40% 以适配更高平台 |
| player | 跳跃起跳 | jump_initial_hold_time | 0.2 | 秒 | 起跳后先保持上升的时长 |
| player | 跳跃重力 | jump_velocity_drop_step | 0.95 | 速度/步 | 每个离散步长的速度降低值 |
| player | 跳跃重力 | jump_velocity_drop_interval | 0.02 | 秒 | 速度降低的结算间隔 |
| player | 跳跃阶段 | jump_total_rise_time | 0.52 | 秒 | 完整起跳后总上升时长 |
| player | 下落阶段 | fall_speed_cap | 20.9 | 速度 | 常规下落最大速度 |
| player | 跳跃结果 | full_jump_height | 5.588 | 高度 | 平地完整一段跳跃的上升距离 |
| player | 跳跃结果 | full_jump_duration | 1.0 | 秒 | 平地完整一段跳跃总时长 |
| player | 跳跃保护 | min_jump_rise_time | 0.08 | 秒 | 即使松键也至少维持的上升时长 |
| player | 提前松键 | early_release_vertical_speed | 0 | 速度 | 最短上升保护结束后松键，纵向速度立即归零 |
| player | 顶头判定 | ceiling_hit_vertical_speed | 0 | 速度 | 整个跳跃过程中撞到天花板时，纵向速度立即归零 |

## 3. 附带规则（非数值）

- 跳跃在最短上升时间结束后，若松开跳跃键则立即转入下落。
- 跳跃过程中若头顶碰到天花板，则立即转入下落。

## 4. 灵魂参数

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| player | 灵魂获取 | soul_gain_per_nail_hit_per_enemy | 11 | 灵魂 | 用骨钉击中敌人时，每命中一个敌人获得的灵魂量 |
| player | 灵魂容量 | soul_meter_max | 99 | 灵魂 | 主灵魂计可容纳的最大灵魂量 |
| player | 灵魂消耗 | spell_cast_soul_cost | 33 | 灵魂 | 施放法术消耗的灵魂量 |
| player | 灵魂消耗 | heal_focus_soul_cost | 33 | 灵魂 | 凝聚回血消耗的灵魂量 |

## 5. 灵魂附带规则

- 灵魂可用于施放伤害性法术，也可用于治疗。
- 灵魂在 HUD 上以圆形仪表盘显示。

## 5.1 NPC / 商店参数

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| npc_chief | 主线提示 | village_chief_hint_count | 1 | 句 | 村长当前至少提供 1 句主线推进提示 |
| npc_doctor | 治疗 | doctor_full_heal_cost | 0 | HKD | 医生当前免费把生命回满 |
| npc_merchant | 商品 | merchant_vital_shell_cost | 40 | HKD | `Vital Shell` 价格，生命上限 +1 |
| npc_merchant | 商品 | merchant_vital_shell_max_purchases | 2 | 次 | 生命强化当前最多购买 2 次 |
| npc_merchant | 商品 | merchant_nail_edge_cost | 55 | HKD | `Nail Edge` 价格，攻击等级 +1 |
| npc_merchant | 商品 | merchant_nail_edge_max_purchases | 2 | 次 | 攻击强化当前最多购买 2 次 |
| npc_merchant | 商品 | merchant_shadow_dash_token_cost | 90 | HKD | `Shadow Dash Token` 价格，影冲能力预留 |

## 5.2 NPC / 商店附带规则

- 医生交互后直接把当前生命补到上限，不做额外前摇。
- 商店当前先做最小闭环：返回商品列表、购买结果和 HKD 扣费。
- `Shadow Dash Token` 当前只返回“已授予影冲解锁标记”的结果，真正技能解锁与存档持久化仍需主流程接线。

## 6. 凝聚回血参数

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| player | 凝聚回血 | heal_focus_first_mask_time | 1.14 | 秒 | 回复第一格血量的总时长，包含额外发动动作 |
| player | 凝聚回血 | heal_focus_next_mask_time | 0.89 | 秒 | 第一格之后，每回复一格血量所需时长 |
| player | 凝聚回血 | heal_focus_hold_threshold | 0.26 | 秒 | 判定为长按并开始凝聚所需时长 |
| player | 凝聚回血 | heal_focus_crouch_start_time | 0.3 | 秒 | 发动凝聚时的俯身前摇时长，此阶段不消耗灵魂 |
| player | 凝聚回血 | heal_focus_channel_time | 0.83 | 秒 | 消耗灵魂并完成一次凝聚回血所需时长 |

## 7. 凝聚回血附带规则

- 凝聚回血的前摇俯身阶段不消耗灵魂。
- 连续凝聚时原设定存在 0.23 秒硬直，松键或回满时原设定存在 0.45 秒硬直。
- 以上两种硬直当前按“可以舍弃”处理，不纳入基础必做调参项。

## 8. 攻击伤害参数

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| player | 骨钉伤害 | nail_damage_level_1 | 5 | 伤害 | 一级骨钉伤害 |
| player | 骨钉伤害 | nail_damage_level_2 | 9 | 伤害 | 二级骨钉伤害，可在铁匠处升级获得 |
| player | 骨钉伤害 | nail_damage_level_3 | 13 | 伤害 | 三级骨钉伤害，可在铁匠处升级获得 |
| player | 骨钉伤害 | nail_damage_starting_value | 5 | 伤害 | 当前项目初始骨钉伤害按 5 处理 |
| player | 法术伤害 | spell_horizontal_damage | 15 | 伤害 | 普通法术向左/右攻击伤害 |
| player | 法术伤害 | spell_down_damage | 20 | 伤害 | 普通法术向下攻击伤害 |
| player | 法术伤害 | spell_up_damage | 22 | 伤害 | 普通法术向上攻击伤害 |

## 9. 攻击伤害附带规则

- 骨钉共有三级伤害档位：5、9、13。
- 后两级骨钉可在铁匠处升级获得。
- 当前项目起始骨钉伤害直接按 5 点处理。

## 10. 地面普通敌人参数

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| enemy_ground | 血量 | ground_enemy_health_current | 3 | 血量 | 地面普通敌人当前实装血量 |
| enemy_ground | 血量 | ground_enemy_health_max | 3 | 血量 | 地面普通敌人最大血量 |
| enemy_ground | 索敌 | ground_enemy_aggro_range | 10 | 格 | 玩家进入该范围后开始追击 |
| enemy_ground | 脱战 | ground_enemy_lose_aggro_range | 16 | 格 | 玩家超出该范围后停止追击并回归出生点 |
| enemy_ground | 巡逻 | ground_enemy_patrol_range | 3 | 格 | 待机时围绕出生点左右巡逻的半径 |
| enemy_ground | 巡逻 | ground_enemy_patrol_pause | 0.20 | 秒 | 巡逻走到端点后短暂停顿时间 |
| enemy_ground | 预警 | ground_enemy_alert_range | 7 | 格 | 玩家进入该范围后触发 `!!!` 预警 |
| enemy_ground | 近战 | ground_enemy_attack_range | 2 | 格 | Dash 攻击命中的有效范围 |
| enemy_ground | 前摇 | ground_enemy_attack_startup | 0.35 | 秒 | `!!!` 预警持续时间 |
| enemy_ground | 冲刺 | ground_enemy_dash_step_time | 0.035 | 秒/格 | Dash 每前进一步的时间 |
| enemy_ground | 恢复 | ground_enemy_attack_recovery | 1.0 | 秒 | 每次攻击完成后的恢复期 |
| enemy_ground | 奖励 | ground_enemy_hkd_reward | 10 | HKD | 玩家击杀地面普通敌人后获得的金钱 |

## 11. 地面普通敌人附带规则

- 地面普通敌人的血条当前按 3 格血量理解。
- 地面普通敌人当前的主要攻击方式为：`!!!` 预警后向玩家方向 Dash。
- 当前击杀地面普通敌人后，玩家会获得 10 HKD，并在玩家头顶短暂显示 `+10` 飘字。

## 12. 飞行敌人参数

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| enemy_flying | 血量 | flying_enemy_health_current | 2 | 血量 | 飞行敌人当前实装血量 |
| enemy_flying | 血量 | flying_enemy_health_max | 2 | 血量 | 飞行敌人最大血量 |
| enemy_flying | 索敌 | flying_enemy_aggro_range | 12 | 格 | 玩家进入该范围后，飞行敌人开始追踪 |
| enemy_flying | 脱战 | flying_enemy_lose_aggro_range | 18 | 格 | 玩家超出该范围后停止追击并回到出生点 |
| enemy_flying | 预警 | flying_enemy_alert_range | 8 | 格 | 玩家进入该范围后触发 `!!!` 并准备发射火球 |
| enemy_flying | 悬停 | flying_enemy_hover_band | 2 | 格 | 围绕出生高度上下浮动的范围 |
| enemy_flying | 移动 | flying_enemy_move_step_time | 0.24 | 秒/步 | 飞行敌人每次位移的步进时间，显著慢于玩家 |
| enemy_flying | 前摇 | flying_enemy_attack_startup | 0.30 | 秒 | 头顶 `!!!` 预警持续时间 |
| enemy_flying | 恢复 | flying_enemy_attack_recovery | 0.80 | 秒 | 每次发射火球后的恢复期 |
| enemy_flying | 弹道 | flying_enemy_projectile_step_frames | 3 | 帧 | 火球每前进一步的间隔 |
| enemy_flying | 伤害 | flying_enemy_fireball_damage | 1 | 伤害 | 火球命中玩家造成 1 点伤害 |
| enemy_flying | 奖励 | flying_enemy_hkd_reward | 10 | HKD | 玩家击杀飞行敌人后获得的金钱 |

## 13. 飞行敌人附带规则

- 飞行敌人当前已按正式模块 demo 方式实现：慢速悬停、索敌追踪、`!!!` 前摇、发射火球、恢复、脱战回位。
- 玩家距离较远时，飞行敌人主要围绕出生点上下悬停。
- 玩家进入索敌范围后，飞行敌人会缓慢贴近并对准高度，再发起火球攻击。
- 飞行敌人同样使用短受击与短死亡反馈，不额外挂血条。

## 14. 敌人受击 / 死亡反馈规则

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| enemy_common | 受击反馈 | enemy_hit_flash_duration | 0.12 | 秒 | 受伤时仅闪一次 `*`，随后立即恢复原字形 |
| enemy_common | 死亡反馈 | enemy_death_flash_duration | 0.07 | 秒 | 死亡第一帧显示 `*` |
| enemy_common | 死亡反馈 | enemy_death_marker_duration | 0.07 | 秒 | 死亡第二帧显示 `x`，随后立即消失 |

## 15. 敌人受击 / 死亡附带规则

- 普通敌人受伤反馈统一按 `g/f -> * -> g/f` 处理，整体保持很短，不做拖沓动画。
- 普通敌人死亡反馈统一按 `g/f -> * -> x -> 空` 处理，2 到 3 个视觉阶段后立即消失。
- 当前阶段先**不做敌人血条**，优先保证受击和死亡反馈足够短、清楚、利落。

## 16. Boss 原型框架参数（当前仅用于 BossSandbox）

| 模块 | 机制 | 参数名 | 数值 | 单位 | 备注 |
| --- | --- | --- | --- | --- | --- |
| boss_common | 硬直窗口 | boss_stagger_window_duration | 5.0 | 秒 | Boss 原型共用的累计受伤硬直窗口 |
| boss_common | 受击反馈 | boss_hit_flash_duration | 0.12 | 秒 | Boss 受伤时短闪 `*` |
| boss_common | 死亡反馈 | boss_death_flash_duration | 0.10 | 秒 | Boss 死亡第一阶段显示 `*` |
| boss_common | 死亡反馈 | boss_death_marker_duration | 0.12 | 秒 | Boss 死亡第二阶段显示 `x` |
| boss_melee | 血量 | boss_melee_health_current | 18 | 血量 | 当前近战 Boss 沙盒原型血量，后续可再调 |
| boss_melee | 硬直阈值 | boss_melee_stagger_threshold | 7 | 伤害 | 5 秒内累计达到该值时进入硬直 |
| boss_melee | 前摇 | boss_melee_attack_startup | 0.45 | 秒 | 近战 Boss 常规攻击前摇 |
| boss_melee | 恢复 | boss_melee_attack_recovery | 0.70 | 秒 | 近战 Boss 攻击后的恢复期 |
| boss_melee | 硬直 | boss_melee_stagger_duration | 1.10 | 秒 | 当前近战 Boss 原型硬直时长 |
| boss_melee | 奖励 | boss_melee_hkd_reward | 60 | HKD | 击杀近战 Boss 原型的奖励 |
| boss_ranged | 血量 | boss_ranged_health_current | 14 | 血量 | 当前远程 Boss 沙盒原型血量，后续可再调 |
| boss_ranged | 硬直阈值 | boss_ranged_stagger_threshold | 6 | 伤害 | 5 秒内累计达到该值时进入硬直 |
| boss_ranged | 前摇 | boss_ranged_attack_startup | 0.40 | 秒 | 远程 Boss 常规施法前摇 |
| boss_ranged | 恢复 | boss_ranged_attack_recovery | 0.75 | 秒 | 远程 Boss 攻击后的恢复期 |
| boss_ranged | 硬直 | boss_ranged_stagger_duration | 1.20 | 秒 | 当前远程 Boss 原型硬直时长 |
| boss_ranged | 奖励 | boss_ranged_hkd_reward | 70 | HKD | 击杀远程 Boss 原型的奖励 |
| boss_sandbox | 陨石预警 | boss_meteor_warning_frames | 18 | 帧 | 陨石打击前的 `^` 预警帧数 |
| boss_sandbox | 陨石爆发 | boss_meteor_blast_frames | 6 | 帧 | 陨石落点爆发显示时长 |

## 17. Boss 原型附带规则（当前仅用于 BossSandbox）

- 当前 Boss 只先搭框架和测试沙盒，不代表最终 Boss 设计数值已经锁定。
- 近战 Boss 原型当前包含：苏醒、贴近、横扫、冲刺斩、受击、硬直、死亡。
- 远程 Boss 原型当前包含：苏醒、拉扯、火球散射、陨石点名、受击、硬直、死亡。
- `BossSandbox` 当前按 `1/2` 刷两种 Boss，`H/J` 用轻击/重击直接验证伤害、奖励和硬直结算。
