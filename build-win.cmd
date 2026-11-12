@echo off
REM Build Convoy for the Commander X16 (cc65 target cx16).
REM x16-lib-poc is consumed as source (model (c)): X16LIB points at the lib checkout and
REM X16LIB_MODULES lists the lib units this game links, compiled with src\x16cfg.h.
REM
REM Usage: build-win.cmd         -> build\CONVOY.PRG, copied to boot\
REM        build-win.cmd smoke   -> build\SMOKE.PRG (boot\ PRG left untouched)
REM Pixel: pixel-cli export writes boot\SPRITES.BIN, boot\PAL.BIN, boot\TILES.BIN.
REM Sound: sound-cli export writes build\gen\sfx_ids.h and build\gen\sfx_sfx.h.
setlocal
set CC65=%~dp0..\..\tools\cc65\win32
set PATH=%CC65%\bin;%PATH%
cd /d "%~dp0"
if "%X16LIB%"=="" set X16LIB=..\..\x16-lib-poc
set X16LIB_MODULES=display fileio\file_load fileio\file_banks vera\vera_addr vera\vera_addr16 vram\vram_upload tilemap\tilemap_stream layers\layer_config_tile layers\layer_enable_in_dc layers\layer_scroll audio\psg_core sprites\sprite_put sprites\sprite_hide sprites\sprite_enable expansion\exp_uart expansion\netcard
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
if "%PIXEL_CLI%"=="" set PIXEL_CLI=..\pixel-x16\PixelCli\bin\Release\net10.0-windows\pixel-cli.exe
if not exist "%PIXEL_CLI%" (echo ERROR: pixel-cli not found at %PIXEL_CLI%. Set PIXEL_CLI or run: dotnet build ..\pixel-x16\PixelCli -c Release & exit /b 1)
"%PIXEL_CLI%" export --project . --resource plot --out boot\SPRITES.BIN --order escort:1,merchant:1,tanker:1,uboat-ii:1,uboat-viic:1,uboat-ixc:1,uboat-xxi:1,select:1,eel:1,burst:1 --palette-out boot\PAL.BIN --bytes 1280
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export-tiles --project . --resource sea --out boot\TILES.BIN --bytes 768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource hulls --out boot\escort.bin --order escort:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource hulls --out boot\merchant.bin --order merchant:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource hulls --out boot\tanker.bin --order tanker:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource hulls --out boot\uboat.bin --order uboat:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource hulls --out boot\wake.bin --order wake:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource profiles --out boot\profesc.bin --order escort:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource profiles --out boot\profmer.bin --order merchant:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource profiles --out boot\proftnk.bin --order tanker:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource profiles --out boot\profubt.bin --order uboat:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource zoom --out boot\zoom.bin --order escort:16,merchant:16,tanker:16,uboat:16,wake:16 --bytes 2560
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource marks --out boot\marks.bin --order eel:1,burst:1 --bytes 256
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource marks --out boot\cross.bin --order cross:1 --bytes 2048
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource scope --out boot\scope.bin --order beam:16,face:1,blip:1,blip-red:1,foam:1,ring:1 --bytes 4992
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource gauges --out boot\gauges.bin --order dial:1,needle:8,lamp-on:1,lamp-off:1,tube-full:1,tube-empty:1 --bytes 6656
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource weapons --out boot\weapons.bin --order eel:16,twake:16,column:1 --bytes 4608
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource mid --out boot\mid.bin --order escort:16,merchant:16,tanker:16,uboat:16 --bytes 32768
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource mid --out boot\midwake.bin --order wake:16 --bytes 8192
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource tiny --out boot\tiny.bin --order escort:16,merchant:16,tanker:16,uboat:16 --bytes 8192
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource tiny --out boot\tinywake.bin --order wake:16 --bytes 2048
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource charge --out boot\charge.bin --order body:16,splash:16 --bytes 4096
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export --project . --resource numbers --out boot\numbers.bin --order num16:4,num8:4 --bytes 640
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export-picture --project . --resource bowesc --out boot\bowesc.bin
if errorlevel 1 exit /b 1
"%PIXEL_CLI%" export-picture --project . --resource bowubt --out boot\bowubt.bin
if errorlevel 1 exit /b 1
python tools\pack_bows.py
if errorlevel 1 exit /b 1
if "%SOUND_CLI%"=="" set SOUND_CLI=..\sound-x16\SoundCli\bin\Release\net10.0\SoundCli.exe
if not exist "%SOUND_CLI%" (echo ERROR: sound-cli not found at %SOUND_CLI%. Set SOUND_CLI or run: dotnet build ..\sound-x16\SoundCli -c Release & exit /b 1)
"%SOUND_CLI%" export --project . --resource sfx
if errorlevel 1 exit /b 1
set LIBOBJS=
for %%m in (%X16LIB_MODULES%) do (
  if exist %X16LIB%\src\%%m.s (cl65 %CFLAGS% -c -o %OBJDIR%\%%~nm.o %X16LIB%\src\%%m.s) else (cl65 %CFLAGS% -c -o %OBJDIR%\%%~nm.o %X16LIB%\src\%%m.c)
  if errorlevel 1 exit /b 1
  call set LIBOBJS=%%LIBOBJS%% %OBJDIR%\%%~nm.o
)
cl65 %CFLAGS% -Cl -Wl -D,__STACKSIZE__=0x0200 -m build\%OUT%.map -o build\%OUT%.PRG %SRCS% %LIBOBJS%
if errorlevel 1 exit /b 1
del /q src\*.o 2>nul
for %%f in (build\%OUT%.PRG) do echo Built build\%OUT%.PRG (%%~zf bytes, budget 36864)
for %%f in (build\%OUT%.PRG) do if %%~zf GTR 36864 (echo ERROR: build\%OUT%.PRG is over the 36864-byte prg_max & exit /b 1)
if /i not "%OUT%"=="CONVOY" goto :done
copy /Y build\CONVOY.PRG boot\CONVOY.PRG >nul
echo Run with: run-win.cmd
:done
endlocal
