$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

Set-Location $PSScriptRoot

$runtimeExe = 'hongkongknight.exe'
$outputDir = Join-Path $PSScriptRoot 'dist'
$outputExe = Join-Path $outputDir 'random_oneclick_play.exe'
$buildRoot = Join-Path $env:TEMP 'random_oneclick_build'
$payloadRoot = Join-Path $buildRoot 'payload'
$packageRoot = Join-Path $buildRoot 'package'
$payloadZip = Join-Path $packageRoot 'payload.zip'
$launcherPath = Join-Path $packageRoot 'launch.bat'
$sedPath = Join-Path $buildRoot 'random_oneclick_play.sed'

if (Test-Path $buildRoot) {
    Remove-Item -LiteralPath $buildRoot -Recurse -Force
}

New-Item -ItemType Directory -Path $outputDir, $payloadRoot, $packageRoot -Force | Out-Null

Write-Host '[1/4] Building runtime...'
& g++ -std=c++11 -Wall -Wextra -O2 -Iinclude `
    src\core\main.cpp `
    src\core\GameSession.cpp `
    src\combat\CombatSystem.cpp `
    src\enemy\Enemy.cpp `
    src\npc\NpcSystem.cpp `
    src\save\SaveSystem.cpp `
    src\world\WorldSystem.cpp `
    src\player\Player.cpp `
    src\world\MapDrawer.cpp `
    src\input\KeyStateManager.cpp `
    -o $runtimeExe

if ($LASTEXITCODE -ne 0) {
    throw 'Runtime build failed.'
}

Write-Host '[2/4] Preparing payload...'
Copy-Item -LiteralPath (Join-Path $PSScriptRoot $runtimeExe) -Destination (Join-Path $payloadRoot $runtimeExe)
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'data') -Destination (Join-Path $payloadRoot 'data') -Recurse

$saveFile = Join-Path $payloadRoot 'data\save_slot_01.sav'
if (Test-Path $saveFile) {
    Remove-Item -LiteralPath $saveFile -Force
}

Compress-Archive -Path (Join-Path $payloadRoot '*') -DestinationPath $payloadZip -Force

$launcher = @'
@echo off
setlocal
set "PKG_DIR=%~dp0"
set "PLAY_ROOT=%TEMP%\random_oneclick_play"
set "GAME_ROOT=%PLAY_ROOT%\game"

if exist "%PLAY_ROOT%" rmdir /s /q "%PLAY_ROOT%"
mkdir "%GAME_ROOT%" >nul 2>&1

powershell -NoProfile -ExecutionPolicy Bypass -Command "Expand-Archive -LiteralPath '%PKG_DIR%payload.zip' -DestinationPath '%GAME_ROOT%' -Force"
if errorlevel 1 (
    echo Failed to unpack game files.
    pause
    exit /b 1
)

cls
pushd "%GAME_ROOT%"
start "" /wait hongkongknight.exe
set "GAME_EXIT=%ERRORLEVEL%"
popd

rmdir /s /q "%PLAY_ROOT%"
exit /b %GAME_EXIT%
'@

Set-Content -LiteralPath $launcherPath -Value $launcher -Encoding ASCII

$sed = @"
[Version]
Class=IEXPRESS
SEDVersion=3

[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=1
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=N
InstallPrompt=
DisplayLicense=
FinishMessage=
TargetName=$outputExe
FriendlyName=Random One Click Play
AppLaunched=cmd.exe /c launch.bat
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
SourceFiles=SourceFiles

[Strings]
FILE0="payload.zip"
FILE1="launch.bat"

[SourceFiles]
SourceFiles0=$packageRoot

[SourceFiles0]
%FILE0%=
%FILE1%=
"@

Set-Content -LiteralPath $sedPath -Value $sed -Encoding ASCII

Write-Host '[3/4] Building one-click package...'
& iexpress /N $sedPath | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw 'IExpress packaging failed.'
}

Write-Host '[4/4] Done.'
Write-Host "Output: $outputExe"

Remove-Item -LiteralPath $buildRoot -Recurse -Force
