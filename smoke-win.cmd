@echo off
REM Smoke: build with -DCONVOY_SMOKE=1, run it in x16emu -warp, and require
REM "convoy smoke ok" on -echo. The PRG runs the plot for 40 ticks and returns to BASIC.
setlocal
cd /d "%~dp0"
set EMUDIR=..\..\tools\x16emu\win64-r49
set EMU=%EMUDIR%\x16emu.exe
call build-win.cmd smoke
if errorlevel 1 exit /b 1
if not exist "%EMU%" (
  echo x16emu not found at %EMU%
  exit /b 1
)
if not exist fixtures mkdir fixtures
if not exist boot mkdir boot
if "%SMOKE_SECS%"=="" set SMOKE_SECS=12
powershell -NoProfile -Command "$p=Start-Process -PassThru -FilePath '%EMU%' -ArgumentList '-rom','%EMUDIR%\rom.bin','-prg','%~dp0build\SMOKE.PRG','-fsroot','%~dp0boot','-echo','-run','-scale','2','-warp','-gif','fixtures\smoke.gif,wait','-sound','none' -RedirectStandardOutput 'build\smoke-echo.txt'; Start-Sleep %SMOKE_SECS%; if(-not $p.HasExited){[void]$p.CloseMainWindow(); if(-not $p.WaitForExit(5000)){$p.Kill()}}"
if errorlevel 1 exit /b 1
echo Wrote fixtures\smoke.gif
findstr /i /c:"convoy smoke load=1 moved=1 hold=1 drift=1" build\smoke-echo.txt && findstr /i /c:"convoy smoke ok" build\smoke-echo.txt
if errorlevel 1 (
  echo CONVOY SMOKE check failed, see build\smoke-echo.txt
  exit /b 1
)
endlocal
