# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -Iinclude
LDFLAGS =

# 目标可执行文件（根据平台自动设置后缀）
ifeq ($(OS),Windows_NT)
    TARGET = hongkongknight.exe
    PLAYER_SANDBOX_TARGET = player_sandbox.exe
    SKILL_SANDBOX_TARGET = skill_sandbox.exe
    COMBAT_SANDBOX_TARGET = combat_sandbox.exe
    ENEMY_SANDBOX_TARGET = enemy_sandbox.exe
    BOSS_SANDBOX_TARGET = boss_sandbox.exe
    WORLD_SANDBOX_TARGET = world_sandbox.exe
    MAP_EDITOR_SANDBOX_TARGET = map_editor_sandbox.exe
else
    TARGET = hongkongknight
    PLAYER_SANDBOX_TARGET = player_sandbox
    SKILL_SANDBOX_TARGET = skill_sandbox
    COMBAT_SANDBOX_TARGET = combat_sandbox
    ENEMY_SANDBOX_TARGET = enemy_sandbox
    BOSS_SANDBOX_TARGET = boss_sandbox
    WORLD_SANDBOX_TARGET = world_sandbox
    MAP_EDITOR_SANDBOX_TARGET = map_editor_sandbox
endif

# 可复用源文件
RUNTIME_SRCS = src/player/Player.cpp \
               src/world/MapDrawer.cpp \
               src/input/KeyStateManager.cpp

# 源文件
SRCS = src/core/main.cpp \
        src/core/GameSession.cpp \
        src/combat/CombatSystem.cpp \
         src/enemy/Enemy.cpp \
        src/npc/NpcSystem.cpp \
        src/save/SaveSystem.cpp \
        src/world/WorldSystem.cpp \
        $(RUNTIME_SRCS)

# 头文件
HEADERS = include/player/Player.h \
          include/world/MapDrawer.h \
          include/input/KeyStateManager.h \
          include/shared/GameTypes.h \
          include/combat/CombatSystem.h \
          include/combat/CombatTuning.h \
          include/combat/SkillSystem.h \
          include/enemy/Enemy.h \
          include/world/WorldSystem.h \
          include/save/SaveSystem.h \
          include/npc/NpcSystem.h \
          include/event/EventSystem.h \
          include/core/GameSession.h \
          include/shared/plan.h

# 对象文件
OBJS = $(SRCS:.cpp=.o)

# 默认目标
all: $(TARGET)

# 沙盒集合
sandboxes: $(PLAYER_SANDBOX_TARGET) $(SKILL_SANDBOX_TARGET) $(COMBAT_SANDBOX_TARGET) $(ENEMY_SANDBOX_TARGET) $(BOSS_SANDBOX_TARGET) $(WORLD_SANDBOX_TARGET) $(MAP_EDITOR_SANDBOX_TARGET)

# 链接可执行文件
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(PLAYER_SANDBOX_TARGET): src/sandbox/PlayerSandbox.cpp src/combat/CombatSystem.cpp src/enemy/Enemy.cpp $(RUNTIME_SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ src/sandbox/PlayerSandbox.cpp src/combat/CombatSystem.cpp src/enemy/Enemy.cpp $(RUNTIME_SRCS) $(LDFLAGS)

$(SKILL_SANDBOX_TARGET): src/sandbox/SkillSandbox.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ src/sandbox/SkillSandbox.cpp $(LDFLAGS)

$(COMBAT_SANDBOX_TARGET): src/sandbox/CombatSandbox.cpp src/combat/CombatSystem.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ src/sandbox/CombatSandbox.cpp src/combat/CombatSystem.cpp $(LDFLAGS)

$(ENEMY_SANDBOX_TARGET): src/sandbox/EnemySandbox.cpp src/combat/CombatSystem.cpp src/enemy/Enemy.cpp $(RUNTIME_SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ src/sandbox/EnemySandbox.cpp src/combat/CombatSystem.cpp src/enemy/Enemy.cpp $(RUNTIME_SRCS) $(LDFLAGS)

$(BOSS_SANDBOX_TARGET): src/sandbox/BossSandbox.cpp src/combat/CombatSystem.cpp src/enemy/Enemy.cpp $(RUNTIME_SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ src/sandbox/BossSandbox.cpp src/combat/CombatSystem.cpp src/enemy/Enemy.cpp $(RUNTIME_SRCS) $(LDFLAGS)

$(WORLD_SANDBOX_TARGET): src/sandbox/WorldSandbox.cpp src/world/WorldSystem.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ src/sandbox/WorldSandbox.cpp src/world/WorldSystem.cpp $(LDFLAGS)

$(MAP_EDITOR_SANDBOX_TARGET): src/sandbox/MapEditorSandbox.cpp src/combat/CombatSystem.cpp src/world/WorldSystem.cpp src/enemy/Enemy.cpp $(RUNTIME_SRCS) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ src/sandbox/MapEditorSandbox.cpp src/combat/CombatSystem.cpp src/world/WorldSystem.cpp src/enemy/Enemy.cpp $(RUNTIME_SRCS) $(LDFLAGS)

# 编译源文件
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 平台无关的清理命令
clean:
ifeq ($(OS),Windows_NT)
	@powershell -NoProfile -Command "Get-ChildItem -Path src -Recurse -Filter *.o -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue"
	@if exist $(TARGET) del /Q $(TARGET) 2>nul
	@if exist $(PLAYER_SANDBOX_TARGET) del /Q $(PLAYER_SANDBOX_TARGET) 2>nul
	@if exist $(SKILL_SANDBOX_TARGET) del /Q $(SKILL_SANDBOX_TARGET) 2>nul
	@if exist $(COMBAT_SANDBOX_TARGET) del /Q $(COMBAT_SANDBOX_TARGET) 2>nul
	@if exist $(ENEMY_SANDBOX_TARGET) del /Q $(ENEMY_SANDBOX_TARGET) 2>nul
	@if exist $(BOSS_SANDBOX_TARGET) del /Q $(BOSS_SANDBOX_TARGET) 2>nul
	@if exist $(WORLD_SANDBOX_TARGET) del /Q $(WORLD_SANDBOX_TARGET) 2>nul
	@if exist $(MAP_EDITOR_SANDBOX_TARGET) del /Q $(MAP_EDITOR_SANDBOX_TARGET) 2>nul
else
	@rm -f $(OBJS) $(TARGET) $(PLAYER_SANDBOX_TARGET) $(SKILL_SANDBOX_TARGET) $(COMBAT_SANDBOX_TARGET) $(ENEMY_SANDBOX_TARGET) $(BOSS_SANDBOX_TARGET) $(WORLD_SANDBOX_TARGET) $(MAP_EDITOR_SANDBOX_TARGET) 2>/dev/null || true
endif
	@echo "清理完成"

# 重新构建
rebuild: clean all

# 运行程序
run: $(TARGET)
ifeq ($(OS),Windows_NT)
	@echo "运行程序..."
	@$(TARGET)
else
	@echo "运行程序..."
	@./$(TARGET)
endif

run-player-sandbox: $(PLAYER_SANDBOX_TARGET)
ifneq ($(OS),Windows_NT)
	@./$(PLAYER_SANDBOX_TARGET)
else
	@$(PLAYER_SANDBOX_TARGET)
endif

run-skill-sandbox: $(SKILL_SANDBOX_TARGET)
ifneq ($(OS),Windows_NT)
	@./$(SKILL_SANDBOX_TARGET)
else
	@$(SKILL_SANDBOX_TARGET)
endif

run-combat-sandbox: $(COMBAT_SANDBOX_TARGET)
ifneq ($(OS),Windows_NT)
	@./$(COMBAT_SANDBOX_TARGET)
else
	@$(COMBAT_SANDBOX_TARGET)
endif

run-enemy-sandbox: $(ENEMY_SANDBOX_TARGET)
ifneq ($(OS),Windows_NT)
	@./$(ENEMY_SANDBOX_TARGET)
else
	@$(ENEMY_SANDBOX_TARGET)
endif

run-boss-sandbox: $(BOSS_SANDBOX_TARGET)
ifneq ($(OS),Windows_NT)
	@./$(BOSS_SANDBOX_TARGET)
else
	@$(BOSS_SANDBOX_TARGET)
endif

run-world-sandbox: $(WORLD_SANDBOX_TARGET)
ifneq ($(OS),Windows_NT)
	@./$(WORLD_SANDBOX_TARGET)
else
	@$(WORLD_SANDBOX_TARGET)
endif

# 调试模式
debug: CXXFLAGS += -g -DDEBUG
debug: rebuild

# 显示项目信息
info:
	@echo "=== 项目信息 ==="
	@echo "操作系统: $(OS)"
	@echo "编译器: $(CXX)"
	@echo "目标文件: $(TARGET)"
	@echo "源文件: $(SRCS)"
	@echo "头文件: $(HEADERS)"

# 显示帮助
help:
	@echo "可用命令:"
	@echo "  make all     - 编译程序 (默认)"
	@echo "  make clean   - 清理生成的文件"
	@echo "  make rebuild - 重新编译"
	@echo "  make run     - 编译并运行"
	@echo "  make sandboxes           - 编译全部沙盒"
	@echo "  make run-player-sandbox  - 运行玩家沙盒"
	@echo "  make run-skill-sandbox   - 运行技能沙盒"
	@echo "  make run-combat-sandbox  - 运行战斗沙盒"
	@echo "  make run-enemy-sandbox   - 运行敌人沙盒"
	@echo "  make run-boss-sandbox    - 运行 Boss 沙盒"
	@echo "  make run-world-sandbox   - 运行世界沙盒"
	@echo "  make sandboxes           - 编译全部沙盒（含地图编辑器）"
	@echo "  make debug   - 编译调试版本"
	@echo "  make info    - 显示项目信息"

# 伪目标
.PHONY: all sandboxes clean rebuild run run-player-sandbox run-skill-sandbox run-combat-sandbox run-enemy-sandbox run-boss-sandbox run-world-sandbox debug info help
