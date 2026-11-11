# x16-lib

Consumption model (c). `X16LIB_MODULES` in `build-win.cmd` and the `Makefile` is the list, compiled with `src/x16cfg.h` (`-DX16LIB_HAVE_CFG`).

| Module | Why |
|---|---|
| `display` | `display_init_640x480` (scale 128, 1:1) |
| `fileio/file_load` | `TILES.BIN`, `SPRITES.BIN`, `PAL.BIN` straight into VRAM |
| `vera/vera_addr`, `vera/vera_addr16` | map fill, sprites, and the sfx engine |
| `layers/layer_config_tile` | layer 0, 64×64 map, 16×16, 4bpp |
| `layers/layer_enable_in_dc` | layer 0 on, layer 1 (KERNAL text) kept |
| `layers/layer_scroll` | sea scroll |
| `audio/psg_core` | voice 0 propeller drone |

The sea pattern is written in `platform.c` (a 64×64 checker of the four tiles).

`X16_NO_SPRITE_POOL` is 1. Sprite attributes are written in `platform.c`. Effects are header-only `x16lib/sfx.h`, fed by `build/gen/sfx_sfx.h` from sound-cli. Rules live in `src/game.c` and do not include VERA or the KERNAL. `tests/test_game.c` links that file with gcc.
