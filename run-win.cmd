@echo off
REM Run Convoy in x16emu r49 (builds first if needed). Host filesystem = boot\.
setlocal
cd /d "%~dp0"
set EMUDIR=..\..\tools\x16emu\win64-r49
set EMU=%EMUDIR%\x16emu.exe
if not exist build\CONVOY.PRG (
  call build-win.cmd
  if errorlevel 1 exit /b 1
)
if not exist "%EMU%" (
  echo x16emu not found at %EMU%
  exit /b 1
)
mkdir boot 2>nul
copy /Y build\CONVOY.PRG boot\CONVOY.PRG >nul
start "" "%EMU%" -rom "%EMUDIR%\rom.bin" -prg "%~dp0boot\CONVOY.PRG" -run -scale 2 -fsroot "%~dp0boot"
endlocal
