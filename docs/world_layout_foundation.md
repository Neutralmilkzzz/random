# 世界地图地基（第一版）

## 目的

这份文档只负责先把世界的**布局坐标**、**地图之间的连接关系**、**单向捷径规则**和**每张图的结构骨架**定下来。

暂时**不**在这里细画平台、敌人、地形细节。具体内容后续由 `MapEditorSandbox` 去摆。

---

## 世界总览

主线路径：

`出生点 -> 模块1 -> 模块2 -> 模块3 -> 模块4 -> 模块5 -> Boss房`

回环捷径：

1. `模块3 -> 出生点`：只能从模块3侧开启，出生点初始不能直接下去
2. `模块5 -> 出生点`：只能从模块5侧开启，出生点初始不能直接过去

支线占位：

1. `模块3 -> random event`

---

## 世界布局坐标表

约定：  
- `x` 轴向右增加  
- `y` 轴向下增加  
- 坐标只用于世界总览和拼图，不直接等于地图内坐标

| 地图 ID | 中文名 | 世界坐标 | 地图类型 | 当前定位 |
| --- | --- | --- | --- | --- |
| `spawn_village` | 出生点 | `(0, 0)` | `SpawnVillage` | 主基地 / 初始安全区 |
| `module_01` | 模块1 | `(1, 0)` | `CombatRoom` | 第一段主线战斗区 |
| `module_02` | 模块2 | `(1, 1)` | `CombatRoom` | 向下推进的过渡区 |
| `module_03` | 模块3 | `(0, 1)` | `CombatRoom` | 中枢连接区 / 第一捷径解锁点 |
| `random_event_01` | random event | `(0, 2)` | `EventRoom` | 未设计事件区占位 |
| `module_04` | 模块4 | `(-1, 1)` | `CombatRoom` | 向左推进的主线区 |
| `module_05` | 模块5 | `(-1, 0)` | `CombatRoom` | Boss 前准备区 / 第二捷径解锁点 |
| `boss_room_01` | boss房 | `(-2, 0)` | `BossRoom` | 当前阶段终点 |

---

## 世界连接表

| 起点地图 | 方向 / 结构 | 终点地图 | 目标出生点 | 初始状态 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `spawn_village` | 右出口 | `module_01` | `from_spawn_village` | 开放 | 主线起点 |
| `module_01` | 左出口 | `spawn_village` | `from_module_01` | 开放 | 返回出生点 |
| `module_01` | 下出口 | `module_02` | `from_module_01` | 开放 | 主线向下推进 |
| `module_02` | 上出口 | `module_01` | `from_module_02` | 开放 | 返回模块1 |
| `module_02` | 左出口 | `module_03` | `from_module_02` | 开放 | 主线转向左 |
| `module_03` | 右出口 | `module_02` | `from_module_03` | 开放 | 返回模块2 |
| `module_03` | 左出口 | `module_04` | `from_module_03` | 开放 | 主线继续向左 |
| `module_04` | 右出口 | `module_03` | `from_module_04` | 开放 | 返回模块3 |
| `module_04` | 上出口 | `module_05` | `from_module_04` | 开放 | 主线上抬 |
| `module_05` | 下出口 | `module_04` | `from_module_05` | 开放 | 返回模块4 |
| `module_05` | 左出口 | `boss_room_01` | `from_module_05` | 开放 | Boss 前最后一段 |
| `boss_room_01` | 右出口 | `module_05` | `from_boss_room_01` | 开放 | Boss 房回退 |
| `module_03` | 上方捷径门 | `spawn_village` | `from_module_03_shortcut` | 锁定 | 只能在模块3侧开启 |
| `module_05` | 右侧捷径门 | `spawn_village` | `from_module_05_shortcut` | 锁定 | 只能在模块5侧开启 |
| `module_03` | 下出口 | `random_event_01` | `from_module_03` | 开放 | 事件区占位 |
| `random_event_01` | 上出口 | `module_03` | `from_random_event_01` | 开放 | 返回模块3 |

---

## 单向捷径门规则

这两扇门建议不要写成“普通开放 transition”，而是保留一个**门状态**。

### 捷径门 A

- 门 ID：`shortcut_spawn_module_03`
- 连接：`spawn_village <-> module_03`
- 初始状态：`locked`
- 规则：
  - 出生点这一侧只能看到门，不能主动打开
  - 模块3这一侧允许交互开启
  - 一旦开启，之后双向通行

### 捷径门 B

- 门 ID：`shortcut_spawn_module_05`
- 连接：`spawn_village <-> module_05`
- 初始状态：`locked`
- 规则：
  - 出生点这一侧只能看到门，不能主动打开
  - 模块5这一侧允许交互开启
  - 一旦开启，之后双向通行

后续如果要接存档，最少记录两个布尔值：

- `shortcut_spawn_module_03_open`
- `shortcut_spawn_module_05_open`

---

## 地图文件命名建议

建议后续就按下面这些文件名落：

- `data\maps\spawn_village.map`
- `data\maps\module_01.map`
- `data\maps\module_02.map`
- `data\maps\module_03.map`
- `data\maps\module_04.map`
- `data\maps\module_05.map`
- `data\maps\random_event_01.map`
- `data\maps\boss_room_01.map`

---

## 每张图的结构骨架

下面写的是**结构职责**，不是最终细节图。

### 1. `spawn_village`

**定位**

- 安全区 / 出生点 / 主基地
- 可保留现有村庄内容：老人、医生、小屋、商人

**必须具备的结构**

- 右侧主出口，通往 `module_01`
- 下方或偏下位置保留一扇“看得见但初始打不开”的捷径门，目标是 `module_03`
- 左侧或上侧可继续保留村庄生活区，不影响主线

**建议出生点**

- `player_start`
- `from_module_01`
- `from_module_03_shortcut`
- `from_module_05_shortcut`
- `from_house_interior`

**结构要求**

- 整体节奏偏安全
- 玩家第一次出生后，默认面向主线右边
- 两扇捷径门都应能在出生点被看见，便于建立“以后能开”的记忆

---

### 2. `module_01`

**定位**

- 第一段主线战斗区
- 负责把玩家从安全区正式带入流程

**必须具备的结构**

- 左入口接 `spawn_village`
- 右侧不再继续延伸，主要出口应放在下方，通往 `module_02`

**建议出生点**

- `from_spawn_village`
- `from_module_02`

**结构要求**

- 整体是“向右推进后，在末端下落/下行”
- 地图形状适合新手熟悉第一轮战斗
- 可以做成偏直线的长房间，尾端接竖向下行口

---

### 3. `module_02`

**定位**

- 主线向下后的承接区
- 起到“把右路转成左路”的作用

**必须具备的结构**

- 上入口接 `module_01`
- 左出口接 `module_03`

**建议出生点**

- `from_module_01`
- `from_module_03`

**结构要求**

- 整体建议做成 L 型或折线型
- 玩家从上进入，完成后往左离开
- 不需要承担捷径逻辑，只做过渡和战斗承接

---

### 4. `module_03`

**定位**

- 中枢连接区
- 第一扇捷径门的解锁地图
- 承担“主线 / 捷径 / 支线事件”的三向分流

**必须具备的结构**

- 右入口接 `module_02`
- 左出口接 `module_04`
- 上方有一扇可开启的捷径门，通往 `spawn_village`
- 下出口接 `random_event_01`

**建议出生点**

- `from_module_02`
- `from_module_04`
- `from_spawn_village_shortcut`
- `from_random_event_01`

**结构要求**

- 这是第一张真正的“节点图”
- 玩家应明显意识到这里有多条路
- 捷径门建议放在视觉上显眼的位置，打开后要有“世界被打通”的感觉

---

### 5. `random_event_01`

**定位**

- 未设计事件图占位
- 当前阶段只保留空间和连接，不急着做内容

**必须具备的结构**

- 上入口接 `module_03`

**建议出生点**

- `from_module_03`

**结构要求**

- 现在可以先做成空白事件房
- 后续可替换成剧情、奖励、陷阱、交易、随机战斗中的任一种

---

### 6. `module_04`

**定位**

- 主线继续向左推进的战斗区
- 负责把中枢图引向 Boss 前区域

**必须具备的结构**

- 右入口接 `module_03`
- 上出口接 `module_05`

**建议出生点**

- `from_module_03`
- `from_module_05`

**结构要求**

- 整体建议是“先横向向左推进，再向上爬升”
- 可以做成一张偏行进感、偏攀爬感的房间

---

### 7. `module_05`

**定位**

- Boss 前准备区
- 第二扇捷径门的解锁地图

**必须具备的结构**

- 下入口接 `module_04`
- 左出口接 `boss_room_01`
- 右侧有一扇可开启的捷径门，通往 `spawn_village`

**建议出生点**

- `from_module_04`
- `from_boss_room_01`
- `from_spawn_village_shortcut`

**结构要求**

- 节奏要明显有“Boss 前缓冲”的感觉
- 右侧捷径门建议靠近玩家视线主要路径，形成回家捷径
- 左侧 Boss 入口应有比较明确的仪式感

---

### 8. `boss_room_01`

**定位**

- 当前阶段的主线终点

**必须具备的结构**

- 右入口接 `module_05`

**建议出生点**

- `from_module_05`

**结构要求**

- 当前先只保留房间骨架
- 暂时不急着定 Boss 机制和最终地形
- 先确保它在世界结构上是一个独立终点

---

## 推荐的后续落地顺序

不是一次做完全部，而是按地基顺序往上盖：

1. 先把 `module_01.map`、`module_02.map`、`module_03.map` 建出来
2. 把 `spawn_village <-> module_01 <-> module_02 <-> module_03` 这段主线跑通
3. 再补 `module_04.map`、`module_05.map`
4. 最后再留 `random_event_01.map` 和 `boss_room_01.map` 的占位
5. 两扇捷径门等世界主路径通了以后再接状态开关

---

## 当前版本结论

这一版先把世界分成了 **8 张地图**：

- 1 张出生点
- 5 张主线模块图
- 1 张事件占位图
- 1 张 Boss 图

这已经足够作为后续 `.map` 文件和世界系统接线的地基，不需要现在就把每张图细画完。
