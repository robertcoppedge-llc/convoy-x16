# x16-lib

Consumption model (c). `X16LIB_MODULES` in `build-win.cmd` and the `Makefile` is the list, compiled with `src/x16cfg.h` (`-DX16LIB_HAVE_CFG`).

| Module | Why |
|---|---|
| `display` | `display_init_640x480` (scale 128, 1:1) |
| `fileio/file_load`, `fileio/file_banks` | art into VRAM and banked RAM |
| `vram/vram_upload` | copy one hull or weapon frame into its sprite slot |
| `tilemap/tilemap_stream` | the sea tile map |
| `vera/vera_addr`, `vera/vera_addr16` | address setup used by the modules above and by sfx |
| `layers/layer_config_tile` | layer 0, 64×64 map, 16×16, 4bpp |
| `layers/layer_enable_in_dc` | layer 0 on, layer 1 (KERNAL text) kept |
| `layers/layer_scroll` | sea scroll |
| `sprites/sprite_put`, `sprite_hide`, `sprite_enable` | sprite attributes and the sprite layer. `SPR_B6` / `SPR_B7` pack the bytes |
| `audio/psg_core` | voice 0 propeller drone |
| `expansion/exp_uart`, `expansion/netcard` | Zimodem on UART B |

`X16_NO_SPRITE_POOL` is 1 because each ship owns a fixed slot. `X16LIB_NETCARD` is 1. Effects are header-only `x16lib/sfx.h`, fed by `build/gen/sfx_sfx.h` from sound-cli. Rules live in `src/game.c` and do not include VERA or the KERNAL. `tests/test_game.c` links that file with gcc.
