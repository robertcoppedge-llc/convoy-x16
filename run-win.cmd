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
REM The r49 binary in tools\ does not accept -netcard and exits on the
REM unknown flag. Use it only when this emulator was built with the card.
set NET=
"%EMU%" -h 2>&1 | findstr /i /c:"-netcard" >nul
if not errorlevel 1 set NET=-netcard
REM /wait keeps the emulator alive. A task that exits will otherwise
REM close the window it just opened.
echo Running. Close the emulator to return.
start /wait "" "%EMU%" -rom "%EMUDIR%\rom.bin" %NET% -prg "%~dp0boot\CONVOY.PRG" -run -scale 2 -fsroot "%~dp0boot"
endlocal
