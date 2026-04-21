# Fallen Soul ASCII Demo

Windows 控制台 ASCII 动作游戏 Demo。当前仓库已经包含主线运行所需的地图与资源文件，适合直接发给队友试玩。

## 直接游玩

### Windows：最简单的试玩方式

1. 下载整个仓库文件夹，或直接下载 GitHub 的 ZIP 并完整解压。
2. **不要只单独拿走 `testcpp1.exe`**，必须保留它旁边的 `data` 文件夹。
3. 在仓库根目录双击 `testcpp1.exe`。

只要目录结构完整，Windows 队友**不需要自己编译**，直接就能玩。

当前运行时依赖的是这些相对路径资源：

- `testcpp1.exe`
- `data\maps\index.txt`
- `data\maps\*.map`

首次游玩时如果没有存档，游戏会自动按 `NEW GAME` 从空档开始；存档文件会写到：

- `data\save_slot_01.sav`

### 单文件一键试玩包

如果你想发一个单独的 Windows 可执行文件，可以在仓库根目录运行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\package_release.ps1
```

生成文件：

```text
dist\random_oneclick_play.exe
```

这个文件双击后会自动解包并启动游戏，更适合临时发给不会看目录结构的人试玩。

## 从源码编译

### Windows

需要先安装 `g++` 并加入 `PATH`。

最简单的方式：

```bat
run.bat
```

它会先编译，再直接运行。

如果你只想手动编译主程序：

```bat
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude src\core\main.cpp src\core\GameSession.cpp src\combat\CombatSystem.cpp src\enemy\Enemy.cpp src\npc\NpcSystem.cpp src\save\SaveSystem.cpp src\world\WorldSystem.cpp src\player\Player.cpp src\world\MapDrawer.cpp src\input\KeyStateManager.cpp -o testcpp1.exe
```

### Makefile

仓库也提供了 `Makefile`：

```bash
make
```

会构建主程序；如果环境支持，也可以用：

```bash
make sandboxes
```

来构建各个沙盒程序。

## 操作说明

### 标题界面

- `W/S`：上下选择
- `Enter`：确认
- `Esc`：退出

### 游戏内

- `A/D`：左右移动
- `Space`：跳跃
- `Shift`：横向 dash
- `J`：物理攻击
- `K`：法术攻击
- `W + J / K`：向上攻击 / 向上法术
- `S + J / K`：向下攻击 / 下砸法术
- `E`：与门、NPC、商店交互
- 商店内 `W/S` 选择，`J` 购买，`E` 关闭

## 给队友试玩时最重要的一句

**是的。**如果你的队友用的是 **Windows**，那么他们只需要：

1. 下载整个项目文件夹
2. 保持 `testcpp1.exe` 和 `data` 文件夹还在原位
3. 双击 `testcpp1.exe`

就可以直接游玩。

## 平台说明

- **Windows**：当前是最直接、最推荐的游玩平台。
- **macOS**：不能直接运行仓库里的 `testcpp1.exe`；需要自己编译对应平台版本，或者借助 Wine / CrossOver。
