"""Write the bridge bows as Pixel picture docs.

assets/pictures/bowesc.x16pic.json and bowubt.x16pic.json are the files to
edit. The build exports them; it does not run this script again.
"""
import json
from pathlib import Path

W, H = 256, 120
CX = 127
# Bottom-centered on the 640x480 bridge. Layer 1 is this picture.
PLACE_X, PLACE_Y = (640 - W) // 2, 480 - H

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets" / "pictures"


def blank():
    return [0] * (W * H)


def put(img, x, y, c):
    if c and 0 <= x < W and 0 <= y < H:
        img[y * W + x] = c


def pack(img):
    s = []
    for i in range(0, W * H, 2):
        s.append(f"{img[i] & 15:x}{img[i + 1] & 15:x}")
    return "".join(s)


def escort():
    img = blank()
    for y in range(8, H):
        u = (y - 8) / (H - 9)
        half = int(4 + u * u * 108)
        if y > 86:
            half += (y - 86) * 2
        half = min(half, 118)
        for x in range(CX - half, CX + half + 1):
            d = abs(x - CX)
            if d >= half - 1:
                put(img, x, y, 2)
            elif d >= half - 4:
                put(img, x, y, 1)
            elif d > half * 2 // 3:
                put(img, x, y, 15 if x > CX and d >= half - 8 else 3)
            else:
                put(img, x, y, 14 if (y // 5) % 2 == 0 else 4)
            if d < 2 and (y & 3) == 0:
                put(img, x, y, 15)
        if y < 40:
            put(img, CX - half - 1, y, 11)
            put(img, CX - half - 2, y, 1)
            put(img, CX + half + 1, y, 11)
            put(img, CX + half + 2, y, 1)
    # Breakwater and the gun, looking down the forecastle.
    for y in range(28, 34):
        span = 18 - (y - 28) * 2
        for x in range(CX - span, CX + span):
            put(img, x, y, 2)
            put(img, x, y - 1, 1)
    for y in range(36, 58):
        for x in range(CX - 11, CX + 12):
            if (x - CX) * (x - CX) + (y - 46) * (y - 46) < 70:
                put(img, x, y, 3)
            if (x - CX) * (x - CX) + (y - 46) * (y - 46) < 22:
                put(img, x, y, 2)
    for y in range(22, 46):
        put(img, CX, y, 2)
        put(img, CX + 1, y, 15)
    for bx in (CX - 28, CX + 26):
        for y in range(70, 78):
            put(img, bx, y, 2)
            put(img, bx + 1, y, 1)
            put(img, bx + 2, y, 2)
    # Hawse pipes.
    for hx in (CX - 10, CX + 8):
        for y in range(16, 20):
            put(img, hx, y, 15)
            put(img, hx + 1, y, 2)
    return img


def uboat():
    img = blank()
    for y in range(22, H):
        u = (y - 22) / (H - 23)
        # Blunt nose, then a long narrow casing.
        half = int(8 + (u ** 0.45) * 34)
        half = min(half, 42)
        for x in range(CX - half, CX + half + 1):
            d = abs(x - CX)
            edge = half - 1
            # Round the first rows into a nose.
            if y < 36:
                nose = int(((y - 22) / 14) * half)
                if d > nose:
                    continue
                edge = nose
            if d >= edge:
                put(img, x, y, 2)
            elif d >= edge - 3:
                put(img, x, y, 15)
            elif d > half // 3:
                put(img, x, y, 9)
            else:
                put(img, x, y, 13)
            if d < 1:
                put(img, x, y, 2)
        if 30 < y < 52 and y % 6 == 0:
            put(img, CX - half + 2, y, 15)
            put(img, CX + half - 2, y, 15)
        if y < 48:
            put(img, CX - half - 1, y, 11)
            put(img, CX + half + 1, y, 11)
    # The tower you are standing in, open toward the bow.
    for y in range(92, H):
        for x in range(CX - 96, CX + 97):
            put(img, x, y, 2)
        slot = 20 + (y - 92)
        for x in range(CX - slot, CX + slot + 1):
            put(img, x, y, 9)
            if abs(x - CX) < 8:
                put(img, x, y, 13)
        if y < 96:
            for x in range(CX - 96, CX + 97):
                if abs(x - CX) > slot:
                    put(img, x, y, 1)
    # Jumping wires from the nose back to the tower.
    for y in range(40, 92):
        w = (y - 40) // 8
        put(img, CX - 6 - w, y, 2)
        put(img, CX + 6 + w, y, 2)
    return img


def doc(resource, img, note):
    return {
        "format": "x16.picture",
        "schema": 1,
        "project": "../../x16project.json",
        "resource": resource,
        "name": resource,
        "palette": "main",
        "mode": "bitmap640",
        "bpp": 4,
        "width": W,
        "height": H,
        "layout": "tiles",
        "placement": {"x": PLACE_X, "y": PLACE_Y},
        "paletteOffset": 0,
        "paletteRange": {"first": 0, "count": 16},
        "notes": note,
        "pixels": pack(img),
    }


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    pics = {
        "bowesc": (
            escort(),
            "Escort forecastle from the bridge. Colour 0 is empty so the sea shows through. Edit this picture; the build slices it onto layer 1.",
        ),
        "bowubt": (
            uboat(),
            "U-boat casing and the conning-tower lip, looking forward. Colour 0 is empty. Edit this picture; the build slices it onto layer 1.",
        ),
    }
    for name, (img, note) in pics.items():
        path = OUT / f"{name}.x16pic.json"
        path.write_text(json.dumps(doc(name, img, note), indent=2) + "\n", encoding="utf-8")
        print(f"Wrote {path} ({W}x{H})")


if __name__ == "__main__":
    main()
