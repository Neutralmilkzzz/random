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
| player | 跳跃起跳 | jump_initial_upward_speed | 15.7 | 速度 | 按下跳跃键后的初始上升速度 |
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
| enemy_ground | 预警 | ground_enemy_alert_range | 7 | 格 | 玩家进入该范围后触发 `!!!` 预警 |
| enemy_ground | 近战 | ground_enemy_attack_range | 2 | 格 | Dash 攻击命中的有效范围 |
| enemy_ground | 前摇 | ground_enemy_attack_startup | 0.35 | 秒 | `!!!` 预警持续时间 |
| enemy_ground | 冲刺 | ground_enemy_dash_step_time | 0.035 | 秒/格 | Dash 每前进一步的时间 |
| enemy_ground | 恢复 | ground_enemy_attack_recovery | 1.0 | 秒 | 每次攻击完成后的恢复期 |

## 11. 地面普通敌人附带规则

- 地面普通敌人的血条当前按 3 格血量理解。
- 地面普通敌人当前的主要攻击方式为：`!!!` 预警后向玩家方向 Dash。
