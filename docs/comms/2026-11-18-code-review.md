# Code review notice: convoy-x16 (2026-11-18)

Who: Cursor, for Robert. Notice of work already on `main`. The full list is `x16-lib-poc/docs/comms/2026-11-18-review-implementation-plan.md`. The standing rules are `x16-lib-poc/docs/X16-PROJECT-START.md` section 12.

Names only, in `src/platform.c` (`0ed3715`, #2). The values did not change. `cl65` compiled the file. The emulator was not run.

- The bow map is `LAYER_MAP_128` by `LAYER_MAP_64`. The play maps are `LAYER_MAP_64` by `LAYER_MAP_64`.
- `phase.bin` loads at `X16_BANK_WIN`.
- `show_card` still calls `layer_scroll(0, 0, 0)` after `layer_config_bitmap`. The lib function does not clear the palette offset.

`AGENTS.md` is committed (`288321b`, #3). The older `feat/balance` branch was left in place. Untracked `assets/sourced/` and `assets/tilemaps/` were not committed. No `RESMAP_SPRITES` table was added.
