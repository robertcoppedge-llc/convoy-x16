@echo off
REM Build Convoy for the Commander X16 (cc65 target cx16).
REM x16-lib-poc is consumed as source (model (c)): X16LIB points at the lib checkout and
REM X16LIB_MODULES lists the lib units this game links, compiled with src\x16cfg.h.
REM
REM Usage: build-win.cmd         -> build\CONVOY.PRG, copied to boot\
REM        build-win.cmd smoke   -> build\SMOKE.PRG (boot\ PRG left untouched)
REM Pixel: pixel-cli export writes boot\PAL.BIN, boot\TILES.BIN, and the sprite banks.
REM Sound: sound-cli export writes build\gen\sfx_ids.h and build\gen\sfx_sfx.h.
setlocal
set CC65=%~dp0..\..\tools\cc65\win32
set PATH=%CC65%\bin;%PATH%
cd /d "%~dp0"
if "%X16LIB%"=="" set X16LIB=..\..\x16-lib-poc
set X16LIB_MODULES=display fileio\file_load fileio\file_banks fileio\file_save fileio\config_load vera\vera_addr vera\vera_addr16 vram\vram_upload tilemap\tilemap_stream layers\layer_config_tile layers\layer_config_bitmap layers\layer_enable_in_dc layers\layer_scroll audio\psg_core sprites\sprite_put sprites\sprite_hide sprites\sprite_enable bank\bank_jsr expansion\exp_uart expansion\netcard
set SRCS=src\main.c src\game.c src\platform.c src\link.c
set CFLAGS=-t cx16 -O -Isrc -Ibuild\gen -I%X16LIB%\include -DX16LIB_HAVE_CFG
set OUT=CONVOY
set OBJDIR=build\x16lib
if /i "%~1"=="smoke" (
  set CFLAGS=%CFLAGS% -DCONVOY_SMOKE=1
  set OUT=SMOKE
  set OBJDIR=build\x16lib-smoke
)
if not exist build\gen mkdir build\gen
if not exist %OBJDIR% mkdir %OBJDIR%
if not exist boot mkdir boot
del /q boot\*.bin 2>nul
if not exist boot\CONVOY.CFG powershell -NoProfile -Command "[IO.File]::WriteAllBytes((Join-Path (Get-Location) 'boot\CONVOY.CFG'), [byte[]](0,0,0x58,0x43,1,2,1,1))"
if "%PIXEL_CLI%"=="" set PIXEL_CLI=..\pixel-x16\PixelCli\bin\Release\net10.0-windows\pixel-cli.exe
if not exist "%PIXEL_CLI%" (echo ERROR: pixel-cli not found at %PIXEL_CLI%. Set PIXEL_CLI or run: dotnet build ..\pixel-x16\PixelCli -c Release & exit /b 1)
"%PIXEL_CLI%" export-palette --project . --resource main --out boot\PAL.BIN
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export-tiles --project . --resource sea --out boot\TILES.BIN --bytes 768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\ESCORT.BIN --order chart-escort:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\MERCHANT.BIN --order chart-merchant:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\TANKER.BIN --order chart-tanker:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\UBOAT.BIN --order chart-uboat:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\PROFESC.BIN --order bridge-escort:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\PROFMER.BIN --order bridge-merchant:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\PROFTNK.BIN --order bridge-tanker:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\PROFUBT.BIN --order bridge-uboat:16 --bytes 32768
if errorlevel 1 exit /b 1
REM Smaller profiles are sheets in game.x16spr.json (p32-*, p16-*, p8-*), made by pixel-cli lod
REM from the bridge-* sheets. Not part of every build. When the bridge art changes, run once per kind:
REM   pixel-cli lod --project . --resource game --sheet bridge-escort --to p32-escort:32,p16-escort:16,p8-escort:8
REM and the same for merchant, tanker, and uboat. Default mode is majority (index 0 stays transparent).
"%PIXEL_CLI%" export --project . --resource game --out boot\P32ESC.BIN --order p32-escort:16 --bytes 8192
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\P32MER.BIN --order p32-merchant:16 --bytes 8192
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\P32TNK.BIN --order p32-tanker:16 --bytes 8192
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\P32UBT.BIN --order p32-uboat:16 --bytes 8192
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\P16PROF.BIN --order p16-escort:16,p16-merchant:16,p16-tanker:16,p16-uboat:16 --bytes 8192
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\P8PROF.BIN --order p8-escort:16,p8-merchant:16,p8-tanker:16,p8-uboat:16 --bytes 2048
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\ZOOM.BIN --order z8-escort:16,z8-merchant:16,z8-tanker:16,z8-uboat:16 --bytes 2048
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\MARKS.BIN --order mark-eel:1,mark-burst:1 --bytes 256
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\CROSS.BIN --order mark-cross:1 --bytes 2048
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\SCOPE.BIN --order scope-beam:16,scope-face:1,scope-blip:1,scope-blip-red:1 --bytes 4160
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\GAUGES.BIN --order gauge-needle:8,gauge-lamp-on:1,gauge-lamp-off:1,gauge-tube-full:1,gauge-tube-empty:1 --bytes 4224
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\WEAPONS.BIN --order weapon-eel:16,weapon-twake:16,weapon-column:1 --bytes 4608
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\MID.BIN --order z32-escort:16,z32-merchant:16,z32-tanker:16,z32-uboat:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\TINY.BIN --order z16-escort:16,z16-merchant:16,z16-tanker:16,z16-uboat:16 --bytes 8192
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\CHARGE.BIN --order charge-body:1,charge-splash:1 --bytes 256
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource game --out boot\NUMBERS.BIN --order num-num16:4,num-num8:4 --bytes 640
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export-picture --project . --resource bowesc --out boot\BOWESC.BIN
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export-picture --project . --resource bowubt --out boot\BOWUBT.BIN
if errorlevel 1 exit /b 1
REM Pack step, Convoy only: tools\pack_bows.py reshapes the two picture exports. It prepends a blank
REM tile, appends a sky tile and a horizon tile (15,360 bytes becomes 15,456), and writes the shared
REM 128x64 map BOWMAP.BIN. Pixel records the pre-pack size. The sky, horizon, and row-21 fill stay here.
python tools\pack_bows.py
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export-picture --project . --resource title --out boot\TITLE.BIN
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export-picture --project . --resource credits --out boot\CREDITS.BIN
if errorlevel 1 exit /b 1
if "%SOUND_CLI%"=="" set SOUND_CLI=..\sound-x16\SoundCli\bin\Release\net10.0\SoundCli.exe
if not exist "%SOUND_CLI%" (echo ERROR: sound-cli not found at %SOUND_CLI%. Set SOUND_CLI or run: dotnet build ..\sound-x16\SoundCli -c Release & exit /b 1)
"%SOUND_CLI%" export --project . --resource sfx
if errorlevel 1 exit /b 1
REM Resource map: src\resmap.h against boot\. Writes build\gen\resmap.inc before the link.
REM PHASE.BIN is copied after the link, so this pass allows it to be missing.
call %X16LIB%\tools\resmap\resmap.cmd src\resmap.h -Boot boot -Doc docs\banking.md -Svg docs\img -AsmInc build\gen\resmap.inc -MissingOk -Quiet || exit /b 1
set LIBOBJS=
for %%m in (%X16LIB_MODULES%) do (
  if exist %X16LIB%\src\%%m.s (cl65 %CFLAGS% -c -o %OBJDIR%\%%~nm.o %X16LIB%\src\%%m.s) else (cl65 %CFLAGS% -c -o %OBJDIR%\%%~nm.o %X16LIB%\src\%%m.c)
  if errorlevel 1 exit /b 1
  call set LIBOBJS=%%LIBOBJS%% %OBJDIR%\%%~nm.o
)
ca65 -t cx16 -I build\gen -o build\phasecall.o src\phasecall.s
if errorlevel 1 exit /b 1
cl65 %CFLAGS% -C src\convoy.cfg -Cl -Wl -D,__STACKSIZE__=0x0200 -m build\%OUT%.map -o build\%OUT%.PRG %SRCS% build\phasecall.o %LIBOBJS%
if errorlevel 1 exit /b 1
copy /Y build\%OUT%.PRG.PHS boot\PHASE.BIN >nul
call %X16LIB%\tools\resmap\resmap.cmd src\resmap.h -Boot boot -Doc docs\banking.md -Svg docs\img -AsmInc build\gen\resmap.inc -Quiet || exit /b 1
del /q src\*.o 2>nul
for %%f in (build\%OUT%.PRG) do echo Built build\%OUT%.PRG (%%~zf bytes, budget 36864)
for %%f in (build\%OUT%.PRG) do if %%~zf GTR 36864 (echo ERROR: build\%OUT%.PRG is over the 36864-byte prg_max & exit /b 1)
if /i not "%OUT%"=="CONVOY" goto :done
copy /Y build\CONVOY.PRG boot\CONVOY.PRG >nul
echo Run with: run-win.cmd
:done
endlocal
