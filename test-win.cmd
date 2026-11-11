@echo off
REM Host unit tests for the plot rules (src\game.c). Needs gcc on PATH.
setlocal
cd /d "%~dp0"
where gcc >nul 2>nul || (echo ERROR: gcc not found on PATH. Install llvm-mingw or WinLibs. & exit /b 1)
if not exist build mkdir build
gcc -O2 -Wall -Wextra -Isrc -o build\test_game.exe tests\test_game.c src\game.c
if errorlevel 1 exit /b 1
build\test_game.exe
if errorlevel 1 exit /b 1
endlocal
