# Fallen Soul ASCII Demo

Windows / Linux / macOS 终端 ASCII 动作游戏 Demo。主线运行依赖仓库内的地图与存档目录，因此请始终保留根目录下的 `data` 文件夹。

## 下载方式

三端通用：

1. 进入 GitHub 仓库页面。
2. 任选一种方式下载：
   - `git clone https://github.com/Neutralmilkzzz/random.git`
   - 或点击 **Code -> Download ZIP**
3. 下载后进入仓库根目录。

## Quick Start

1. **Windows**：直接双击 `hongkongknight.exe`，或者运行 `run.bat`
2. **Linux / macOS**：在仓库根目录执行 `make`，然后运行 `./hongkongknight`
3. 进入标题页后用 `W/S` 选项、`Enter` 确认；`Esc` 会打开退出确认菜单，不再直接关闭游戏

## Windows 运行

如果仓库里已经带着编好的 `hongkongknight.exe`，Windows 队友可以直接：

1. 下载整个项目文件夹
2. 保持 `hongkongknight.exe` 和 `data` 文件夹在同一级目录
3. 双击 `hongkongknight.exe`

如果本地没有现成 exe，就先安装 `g++`，然后运行：

```bat
run.bat
```

或者手动编译：

```bat
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude src\core\main.cpp src\core\GameSession.cpp src\combat\CombatSystem.cpp src\enemy\Enemy.cpp src\npc\NpcSystem.cpp src\save\SaveSystem.cpp src\world\WorldSystem.cpp src\player\Player.cpp src\world\MapDrawer.cpp src\input\KeyStateManager.cpp -o hongkongknight.exe
```

## Linux 运行

### 1. 安装编译工具

常见发行版可以先安装 `git`、`g++` 和 `make`：

```bash
# Ubuntu / Debian
sudo apt update
sudo apt install git g++ make

# Fedora
sudo dnf install git gcc-c++ make

# Arch
sudo pacman -S git gcc make
```

### 2. 下载仓库

```bash
git clone https://github.com/Neutralmilkzzz/random.git
cd random
```

如果你是下载 ZIP，就先解压再 `cd` 进去。**后面所有命令都必须在这个仓库根目录里执行**，不要切到 `src/` 里面运行。

### 3. Linux 服务器一步一步启动

如果你现在就在自己的 Linux 服务器上，最稳的一套命令就是：

```bash
cd /你的/项目路径/random
git pull
ls data/maps/index.txt
make
./hongkongknight
```

说明：

1. `cd /你的/项目路径/random`：进入仓库根目录
2. `git pull`：拉最新代码；如果你是刚下载的 ZIP，可以跳过
3. `ls data/maps/index.txt`：确认地图资源还在
4. `make`：编译 Linux 版可执行文件 `hongkongknight`
5. `./hongkongknight`：启动游戏

如果你看到类似：

```text
Failed to open map file: data/maps/spawn_village.map
```

通常说明两件事之一：

1. 你**不在仓库根目录**
2. 你的 `data/maps` 文件夹被删了或者没下载完整

### 4. 编译并运行

最简单：

```bash
make
./hongkongknight
```

或者一步到位：

```bash
make run
```

如果你不用 `make`，也可以手动编译：

```bash
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude src/core/main.cpp src/core/GameSession.cpp src/combat/CombatSystem.cpp src/enemy/Enemy.cpp src/npc/NpcSystem.cpp src/save/SaveSystem.cpp src/world/WorldSystem.cpp src/player/Player.cpp src/world/MapDrawer.cpp src/input/KeyStateManager.cpp -o hongkongknight
./hongkongknight
```

## macOS 运行

### 1. 安装命令行编译工具

先安装 Apple Command Line Tools：

```bash
xcode-select --install
```

安装完成后确认有 `clang++` 或 `g++`、`make` 可用。

### 2. 下载仓库

```bash
git clone https://github.com/Neutralmilkzzz/random.git
cd random
```

如果你是下载 ZIP，就先解压再进入目录。

### 3. 编译并运行

```bash
make
./hongkongknight
```

或者：

```bash
make run
```

如果你想手动编译：

```bash
c++ -std=c++11 -Wall -Wextra -O2 -Iinclude src/core/main.cpp src/core/GameSession.cpp src/combat/CombatSystem.cpp src/enemy/Enemy.cpp src/npc/NpcSystem.cpp src/save/SaveSystem.cpp src/world/WorldSystem.cpp src/player/Player.cpp src/world/MapDrawer.cpp src/input/KeyStateManager.cpp -o hongkongknight
./hongkongknight
```

## 操作说明

### 标题界面

- `W/S`：上下选择
- `Enter`：确认
- `Esc`：打开退出确认菜单

### 游戏内

#### Windows

- `A/D`：左右移动
- `Space`：跳跃
- `Shift`：横向 dash
- `J`：前方普攻
- `U`：向上劈砍
- `N`：向下劈砍
- `K`：前方法术
- `I`：向上法术
- `M`：向下法术
- `R`：治疗
- `P`：重置当前房间
- `Esc`：打开暂停 / 退出确认菜单
- `E`：与门、NPC、商店交互

#### Linux / macOS

- `A/D`：左右移动
- `Space`：跳跃
- `Q`：横向 dash
- `J`：前方普攻
- `U`：向上劈砍
- `N`：向下劈砍
- `K`：前方法术
- `I`：向上法术
- `M`：向下法术
- `R`：治疗
- `P`：重置当前房间
- `Esc`：打开暂停 / 退出确认菜单
- `E`：与门、NPC、商店交互

商店内三端统一：`W/S` 选择，`J` 购买，`E` 关闭。

## 运行时依赖

主线运行依赖这些相对路径资源：

- Windows：`hongkongknight.exe`
- Linux / macOS：`hongkongknight`
- `data/maps/index.txt`
- `data/maps/*.map`

首次游玩如果没有存档，会从 `NEW GAME` 空档开始；存档写入：

- `data/save_slot_01.sav`
