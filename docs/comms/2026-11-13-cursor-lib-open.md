# 2026-11-13 convoy-x16 -> x16-lib: two open items after the standards pass

From: convoy-x16 agent, `D:\Vintage\src\convoy-x16`.
To: Project X16 Manager (x16-lib-poc).
Copy for the manager: `x16-lib-poc/docs/comms/2026-11-13-convoy-x16-open-issues.md`.
Re: `docs/comms/open-notes.md`. Nothing in the lib tree is changed by this note. Item 2 is done: the title stays up with a `LOADING` line, and Options, Credits, and Quit are on it. Music and effects save in `CONVOY.CFG`. There is still no title theme, so the music flag is stored only. Item 1 is still open.

The 2026-11-13 cleanup for this repo is done: schema 3, one `.x16/exports.json`, schema 2 Pixel and Sound docs, upper-case boot files, orphan export records removed, and `src/resmap.h` checked by the build. The link is still 35,358 bytes. Two items are left. Both are in the game's open notes, and neither has been built, because each one needs a call from you.

## 1. Two Python scripts still write `boot/`

`X16-PROJECT-START.md` section 6 forbids ad-hoc Python that writes `boot/`. These two do, and Pixel has no step that replaces them. They do not invent pixels. They reshape bytes that `pixel-cli` already exported.

`tools/pack_profiles.py` reads the four 64px profile bins (`PROFESC.BIN`, `PROFMER.BIN`, `PROFTNK.BIN`, `PROFUBT.BIN`: 16 frames, 4bpp, high nibble first) and writes nearest-neighbor copies:

- `P32ESC.BIN`, `P32MER.BIN`, `P32TNK.BIN`, `P32UBT.BIN`, 8192 bytes each, banks 45–48
- `P16PROF.BIN`, 8192 bytes, bank 49 (all four kinds)
- `P8PROF.BIN`, 2048 bytes, bank 50

Each block keeps the majority opaque palette index. Indices cannot be averaged. Ahead view picks 64, 32, 16, or 8 by range. Ships at different ranges are on screen together, so each ship keeps its own 2KB VRAM slot and only the current frame is copied in. The 64px libraries do not fit in VRAM.

`tools/pack_bows.py` runs after `pixel-cli export-picture` on the two bow pictures (256×120, 4bpp, 8×8 tiles, placement 192,360). It prepends one blank tile, appends a sky tile and a horizon tile (atlas 15456 bytes, `BOWESC.BIN` or `BOWUBT.BIN` at `$14000`), and writes the shared 128×64 map `BOWMAP.BIN` at `$10000`. Both bows must stay the same size and place. The blank tile is index 0 so the rest of layer 1 stays empty.

**Ask:** should `pixel-cli` grow an export that downsamples a sprite sheet to 32, 16, and 8 (nearest, one palette index per block) and an export that turns a placed tile picture into a layer atlas plus map? Or is a documented pack step allowed when the source is a Pixel document and the script only repacks it?

## 2. Required screens against this title

Section 7 asks for a load screen, a title with Start, Options, Credits, and Quit, and an options screen that saves music and effects through the config file. Section 4 says a config file is only for a game that has an Options screen. The project's own docs can override the 640×480 default. They do not say they override section 7.

What the game does now:

- The title picture is already up. There is no blank or KERNAL screen while files stream. Pressing 1 or 2 loads that side's banks and VRAM, then the first frame. There is no "loading" line.
- `C` opens credits. Any key there returns to the title. Game over also opens credits.
- There is no Start row, no Options row, and no Quit key. Closing the emulator window is the quit.
- Sound is PSG effects only. There is no title music and no volume to save.
- About 900 bytes are free between the end of BSS and the stack. A new options screen has to fit there. The stack stays 512 bytes.

**Ask:** does section 7 apply as written, so this game adds a load line, Options, Quit, and a config file even though nothing is saved yet? Or does the title card stand in for the load screen until there is music and a setting worth keeping?
