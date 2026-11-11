# convoy-x16

A one-screen Atlantic convoy fight for the Commander X16. One player cons up to four escorts. The other cons a U-boat (Type II, VIIC, IXC, or XXI). The screen is the battle space: ships move, and the sea scrolls by the convoy's own speed.

The design lives in [docs/game-plan.md](docs/game-plan.md). This tree is the first slice: the plot, helm, telegraph, station-keeping, and both boats on one keyboard.

## Build and run (Windows)

```text
cd D:\Vintage\src\convoy-x16
build-win.cmd          -> build\CONVOY.PRG (copied to boot\)
run-win.cmd            -> x16emu r49, host filesystem boot\
test-win.cmd           -> host unit tests (gcc)
smoke-win.cmd          -> scripted plot, prints "convoy smoke ok"
```

`make` wraps the same scripts. cc65, x16emu r49, pixel-cli, and sound-cli resolve the same way as the other X16 apps (`..\..\tools`, `..\pixel-x16`, `..\sound-x16`).

## Keys

- `1`–`4` take the conn of that escort. The other three hold station.
- Arrow left/right helm, up/down telegraph (stop, slow, half, full, flank).
- `J` / `L` U-boat helm, `I` / `K` U-boat telegraph.
- `T` cycles the U-boat hull. The sprite and the HUD knots change. The plot does not reset.

## Tools

Project-X16 reads `x16project.json`. Pixel owns `assets/palettes/main.x16pal.json`, `assets/tiles/sea.x16tile.json`, and `assets/sprites/plot.x16spr.json`. Sound owns `assets/sounds/sfx.x16snd.json`. The build exports them; it does not draw its own art.

Display is 640×480, 4bpp tiles and sprites. A full-screen 4bpp framebuffer is 150KB and does not fit in VERA's 128KB, so the sea is a tile layer and the ships are sprites. Index 0 is black and index 1 is white so the KERNAL text on layer 1 stays readable.
