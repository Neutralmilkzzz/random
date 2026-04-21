@echo off
setlocal

cd /d "%~dp0"

where g++ >nul 2>&1
if errorlevel 1 (
    echo [ERROR] g++ not found in PATH.
    echo Install MinGW/GCC first, or add g++ to PATH.
    pause
    exit /b 1
)

echo [1/2] Compiling...
g++ -std=c++11 -Wall -Wextra -O2 -Iinclude src\core\main.cpp src\core\GameSession.cpp src\combat\CombatSystem.cpp src\enemy\Enemy.cpp src\npc\NpcSystem.cpp src\save\SaveSystem.cpp src\world\WorldSystem.cpp src\player\Player.cpp src\world\MapDrawer.cpp src\input\KeyStateManager.cpp -o testcpp1.exe
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo.
echo [2/2] Running...
testcpp1.exe

echo.
echo Program exited.
pause
