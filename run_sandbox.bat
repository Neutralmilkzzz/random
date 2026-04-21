@echo off
setlocal

cd /d "%~dp0"

where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERROR] g++ not found in PATH.
    pause
    exit /b 1
)

set "SANDBOX=%~1"
if "%SANDBOX%"=="" (
    echo Usage: run_sandbox.bat [player^|skill^|combat^|enemy^|world^|editor]
    echo Example: run_sandbox.bat player
    pause
    exit /b 1
)

set "TARGET="
set "ENTRY="
set "EXTRA_SRCS="

if /I "%SANDBOX%"=="player" (
    set "TARGET=player_sandbox.exe"
    set "ENTRY=src\sandbox\PlayerSandbox.cpp"
    set "EXTRA_SRCS=src\enemy\Enemy.cpp src\player\Player.cpp src\world\MapDrawer.cpp src\input\KeyStateManager.cpp"
)

if /I "%SANDBOX%"=="skill" (
    set "TARGET=skill_sandbox.exe"
    set "ENTRY=src\sandbox\SkillSandbox.cpp"
)

if /I "%SANDBOX%"=="combat" (
    set "TARGET=combat_sandbox.exe"
    set "ENTRY=src\sandbox\CombatSandbox.cpp"
)

if /I "%SANDBOX%"=="enemy" (
    set "TARGET=enemy_sandbox.exe"
    set "ENTRY=src\sandbox\EnemySandbox.cpp"
    set "EXTRA_SRCS=src\enemy\Enemy.cpp src\player\Player.cpp src\world\MapDrawer.cpp src\input\KeyStateManager.cpp"
)

if /I "%SANDBOX%"=="world" (
    set "TARGET=world_sandbox.exe"
    set "ENTRY=src\sandbox\WorldSandbox.cpp"
    set "EXTRA_SRCS=src\world\WorldSystem.cpp"
)

if /I "%SANDBOX%"=="editor" (
    set "TARGET=map_editor_sandbox.exe"
    set "ENTRY=src\sandbox\MapEditorSandbox.cpp"
    set "EXTRA_SRCS=src\world\WorldSystem.cpp src\enemy\Enemy.cpp src\player\Player.cpp src\world\MapDrawer.cpp src\input\KeyStateManager.cpp"
)

if "%ENTRY%"=="" (
    echo [ERROR] Unknown sandbox: %SANDBOX%
    echo Available: player, skill, combat, enemy, world, editor
    pause
    exit /b 1
)

echo [1/2] Compiling %SANDBOX% sandbox...
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude %ENTRY% %EXTRA_SRCS% -o %TARGET%
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo [2/2] Running %SANDBOX% sandbox...
%TARGET%

echo.
echo Sandbox exited.
pause
