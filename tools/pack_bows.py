"""Turn the exported bow pictures into a layer-1 tile atlas and map.

pixel-cli export-picture writes 8x8 tiles, row by row. This prepends one
blank tile (index 0, so the rest of the screen stays empty) and writes the
shared 128x64 map from the picture's placement. Both bows must be the same
size and sit in the same place.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BOOT = ROOT / "boot"
PICS = ROOT / "assets" / "pictures"


def load(name):
    return json.loads((PICS / name).read_text(encoding="utf-8"))


def main():
    docs = [load("bowesc.x16pic.json"), load("bowubt.x16pic.json")]
    w, h = docs[0]["width"], docs[0]["height"]
    x, y = docs[0]["placement"]["x"], docs[0]["placement"]["y"]
    for d in docs:
        if (d["width"], d["height"], d["placement"]["x"], d["placement"]["y"]) != (w, h, x, y):
            sys.exit("bowesc and bowubt must share width, height, and placement")
        if d.get("layout") != "tiles" or d.get("bpp") != 4:
            sys.exit(d["resource"] + " must be a 4bpp tile picture")
    if w % 8 or h % 8 or x % 8 or y % 8:
        sys.exit("bow size and placement must sit on an 8 pixel grid")
    cols, rows = w // 8, h // 8
    want = w * h // 2
    blank = bytes(32)
    count = want // 32
    sky = bytes([0x88]) * 32
    horizon = bytes([0x88]) * 24 + bytes([0x77]) * 4 + bytes([0x11]) * 4
    for name in ("bowesc.bin", "bowubt.bin"):
        raw = (BOOT / name).read_bytes()
        if len(raw) != want:
            sys.exit(f"{name} is {len(raw)} bytes, want {want}")
        (BOOT / name).write_bytes(blank + raw + sky + horizon)
    sky_i = 1 + count
    hor_i = sky_i + 1
    mw, mh = 128, 64
    m = bytearray(mw * mh * 2)
    # Sky down to the bridge horizon (y=168, tile row 21). The picture is stamped after.
    for ty in range(20):
        for tx in range(mw):
            cell = ty * mw + tx
            m[cell * 2] = sky_i & 255
            m[cell * 2 + 1] = (sky_i >> 8) & 3
    for tx in range(mw):
        cell = 20 * mw + tx
        m[cell * 2] = hor_i & 255
        m[cell * 2 + 1] = (hor_i >> 8) & 3
    oc, orow = x // 8, y // 8
    if oc + cols > mw or orow + rows > mh:
        sys.exit("bow does not fit on the 128x64 tile map")
    for ty in range(rows):
        for tx in range(cols):
            idx = ty * cols + tx + 1
            cell = (orow + ty) * mw + (oc + tx)
            m[cell * 2] = idx & 255
            m[cell * 2 + 1] = (idx >> 8) & 3
    (BOOT / "bowmap.bin").write_bytes(m)
    print(f"bow layer {cols}x{rows} tiles at {oc},{orow}, atlas {want + 96} bytes")


if __name__ == "__main__":
    main()
