# convoy-x16 (cc65). Windows scripts do the cc65 build, asset export and emulator runs.
# make wraps them and runs the host unit tests with gcc.
CC65_HOME ?= ../../tools/cc65
CL65 ?= $(shell if [ -x "$(CC65_HOME)/bin/cl65" ]; then echo "$(CC65_HOME)/bin/cl65"; \
	elif [ -x "$(CC65_HOME)/win32/bin/cl65.exe" ]; then echo "$(CC65_HOME)/win32/bin/cl65.exe"; \
	elif command -v cl65 >/dev/null 2>&1; then command -v cl65; \
	else echo cl65; fi)

X16EMU ?= $(shell if [ -x "../../tools/x16emu/linux-x86_64-r49/x16emu" ]; then echo "../../tools/x16emu/linux-x86_64-r49/x16emu"; \
	elif [ -x "../../tools/x16emu/win64-r49/x16emu.exe" ]; then echo "../../tools/x16emu/win64-r49/x16emu.exe"; \
	else echo x16emu; fi)

BUILD ?= build
TARGET = $(BUILD)/CONVOY.PRG
X16LIB ?= ../../x16-lib-poc
X16LIB_MODULES = display fileio/file_load fileio/file_banks fileio/file_save fileio/config_load vera/vera_addr vera/vera_addr16 vram/vram_upload tilemap/tilemap_stream layers/layer_config_tile layers/layer_config_bitmap layers/layer_enable_in_dc layers/layer_scroll audio/psg_core sprites/sprite_put sprites/sprite_hide sprites/sprite_enable bank/bank_jsr expansion/exp_uart expansion/netcard
CFLAGS = -t cx16 -O -Isrc -I$(BUILD)/gen -I$(X16LIB)/include -DX16LIB_HAVE_CFG
LDSTACK = -Cl -Wl -D,__STACKSIZE__=0x0200
SRCS = src/main.c src/game.c src/platform.c
PIXEL_CLI ?= ../pixel-x16/PixelCli/bin/Release/net10.0-windows/pixel-cli.exe
SOUND_CLI ?= ../sound-x16/SoundCli/bin/Release/net10.0/SoundCli.exe

.PHONY: all run smoke test clean

all:
	cmd /d /c build-win.cmd

run: all
	cmd /d /c run-win.cmd

smoke:
	cmd /d /c smoke-win.cmd

HOSTCC ?= gcc
test:
	mkdir -p $(BUILD)
	$(HOSTCC) -O2 -Wall -Wextra -Isrc -o $(BUILD)/test_game tests/test_game.c src/game.c
	$(BUILD)/test_game

clean:
	rm -rf $(BUILD) boot *.o *.map
