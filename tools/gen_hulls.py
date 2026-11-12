"""64x64 overhead aspects. 16 frames, frame 0 bow-north, then clockwise.

The helm is 256 steps, so 16 drawings sit on an even 22.5 degrees.
A 16-degree step does not divide 360 or 256.

Each file is 16 * 2048 bytes, 4bpp, index 0 transparent.
"""
import math
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BOOT = os.path.join(ROOT, "boot")
FRAME = 64 * 64 // 2  # 2048

# kind: length, beam, fill, deck, band or 0
KINDS = {
    "escort":  (50, 12, 3, 14, 0),
    "merchant": (44, 18, 4, 14, 0),
    "tanker":  (52, 16, 4, 14, 5),
    "uboat":   (46, 8, 9, 13, 0),
}


def rot_local(x, y, ang):
    dx = x - 31.5
    dy = y - 31.5
    c = math.cos(ang)
    s = math.sin(ang)
    lx = dx * c + dy * s
    ly = dx * s - dy * c
    return lx, ly


def hull_color(lx, ly, length, beam, fill, deck, band):
    if ly < -length / 2 or ly > length / 2:
        return 0
    t = (ly + length / 2) / length
    if t > 0.78:
        half = (beam / 2) * (1.0 - (t - 0.78) / 0.22)
    elif t < 0.12:
        half = (beam / 2) * (0.45 + 0.55 * (t / 0.12))
    else:
        half = beam / 2
    ax = abs(lx)
    if ax > half:
        return 0
    if ax > half - 1.4:
        return 2
    if band and 0.42 < t < 0.55:
        return band
    if ax < 1.2 and 0.2 < t < 0.85:
        return deck
    return fill


def paint_hull(length, beam, fill, deck, band):
    out = bytearray(16 * FRAME)
    for frame in range(16):
        ang = frame * (math.pi / 8.0)
        pix = bytearray(64 * 64)
        for y in range(64):
            for x in range(64):
                lx, ly = rot_local(x, y, ang)
                pix[y * 64 + x] = hull_color(lx, ly, length, beam, fill, deck, band)
        pack(out, frame * FRAME, pix)
    return out


def paint_wake():
    out = bytearray(16 * FRAME)
    for frame in range(16):
        ang = frame * (math.pi / 8.0)
        pix = bytearray(64 * 64)
        for y in range(64):
            for x in range(64):
                lx, ly = rot_local(x, y, ang)
                # Open astern (negative ly). Two foam arms.
                if ly > -2 or ly < -26:
                    continue
                arm = (-ly) * 0.55
                gap = (-ly) * 0.22
                ax = abs(lx)
                if gap < ax < arm and ax < 14:
                    pix[y * 64 + x] = 11 if ax > arm - 1.6 else 1
        pack(out, frame * FRAME, pix)
    return out


def paint_marks():
    """Two 16x16 images: torpedo, burst. 256 bytes."""
    def blank():
        return bytearray(16 * 16)

    eel = blank()
    burst = blank()
    for y in range(16):
        for x in range(16):
            if 7 <= x <= 8 and 2 <= y <= 13:
                eel[y * 16 + x] = 10
            dx, dy = x - 7.5, y - 7.5
            d = dx * dx + dy * dy
            if d <= 36:
                burst[y * 16 + x] = 12 if d > 16 else 1
    raw = bytearray(256)
    pack(raw, 0, eel)
    pack(raw, 128, burst)
    return raw


def pack(dest, offset, pix):
    for i in range(0, len(pix), 2):
        dest[offset + i // 2] = (pix[i] << 4) | (pix[i + 1] & 0x0F)


def sea_pixels():
    def row(s):
        if len(s) != 16:
            raise SystemExit("row %s" % s)
        return s

    solid = "7" * 256
    wave = (
        "7" * 16 * 5
        + row("7777b888888b7777")
        + row("7777788888888777")
        + row("7777778888887777")
        + "7" * 16 * 8
    )
    break_ = (
        "7" * 16 * 11
        + row("77777777778b8777")
        + row("7777777777b88777")
        + "7" * 16 * 3
    )
    return solid, wave, break_, solid


def write_sea():
    path = os.path.join(ROOT, "assets", "tiles", "sea.x16tile.json")
    text = open(path, "r", encoding="utf-8").read()
    pixels = sea_pixels()
    # Replace the four "pixels" strings in order.
    start = 0
    for pix in pixels:
        key = '"pixels": "'
        i = text.find(key, start)
        if i < 0:
            raise SystemExit("pixels field missing")
        j = text.find('"', i + len(key))
        text = text[: i + len(key)] + pix + text[j:]
        start = i + len(key) + len(pix)
    open(path, "w", encoding="utf-8", newline="\n").write(text)


def paint_small(length, beam, fill, deck, band):
    """16 aspects at 8x8, 32 bytes each, for the zoomed chart."""
    out = bytearray(16 * 32)
    for frame in range(16):
        ang = frame * (math.pi / 8.0)
        pix = bytearray(64)
        for y in range(8):
            for x in range(8):
                lx, ly = rot_local(x * 8 + 4, y * 8 + 4, ang)
                pix[y * 8 + x] = hull_color(lx, ly, length, beam, fill, deck, band)
        pack(out, frame * 32, pix)
    return out


def put(pix, x, y, col):
    if 0 <= x < 64 and 0 <= y < 64 and col:
        pix[y * 64 + x] = col


def fill_rect(pix, x0, y0, x1, y1, col):
    if x0 > x1:
        x0, x1 = x1, x0
    if y0 > y1:
        y0, y1 = y1, y0
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put(pix, x, y, col)


def paint_profile(length, beam, fill, deck, band):
    """Four views, waterline at row 48: bow, quarter, beam (bow right), stern.

    Drawn large enough to read at 64, 32, and 16 pixels.
    """
    del length, beam
    out = bytearray(4 * FRAME)

    def pack_frame(n, pix):
        pack(out, n * FRAME, pix)

    bow = bytearray(64 * 64)
    for y in range(22, 48):
        half = 5 + (y - 22) * 10 // 26
        fill_rect(bow, 32 - half, y, 32 + half, y, 2 if y < 24 or y > 45 else fill)
        if 30 <= y <= 44:
            fill_rect(bow, 31, y, 32, y, deck)
    fill_rect(bow, 28, 16, 35, 24, deck)
    fill_rect(bow, 30, 8, 33, 16, 2)
    for y in range(48, 58):
        half = max(2, 14 - (y - 48) * 2)
        fill_rect(bow, 32 - half, y, 32 + half, y, 15)
    fill_rect(bow, 18, 47, 22, 49, 1)
    fill_rect(bow, 41, 47, 45, 49, 1)
    pack_frame(0, bow)

    quarter = bytearray(64 * 64)
    for i in range(36):
        x = 14 + i
        y = 44 - i // 3
        w = 7 + (12 - abs(i - 18)) // 3
        fill_rect(quarter, x, y - 2, x + w, y + 4, fill)
        fill_rect(quarter, x, y + 4, x + w, y + 7, 15)
    fill_rect(quarter, 36, 28, 44, 36, deck)
    fill_rect(quarter, 40, 20, 43, 28, 2)
    if band:
        fill_rect(quarter, 30, 40, 46, 42, band)
    fill_rect(quarter, 8, 46, 16, 50, 1)
    pack_frame(1, quarter)

    beam = bytearray(64 * 64)
    fill_rect(beam, 8, 36, 54, 46, fill)
    fill_rect(beam, 54, 38, 60, 44, fill)
    fill_rect(beam, 8, 46, 56, 52, 15)
    fill_rect(beam, 24, 28, 34, 36, deck)
    fill_rect(beam, 26, 20, 30, 28, 2 if not band else band)
    fill_rect(beam, 36, 30, 44, 36, deck)
    fill_rect(beam, 40, 14, 42, 30, 2)
    fill_rect(beam, 4, 44, 10, 48, 1)
    fill_rect(beam, 2, 46, 6, 50, 1)
    pack_frame(2, beam)

    stern = bytearray(64 * 64)
    fill_rect(stern, 18, 26, 45, 46, fill)
    fill_rect(stern, 22, 18, 41, 26, deck)
    fill_rect(stern, 30, 10, 33, 18, 2)
    fill_rect(stern, 20, 46, 43, 54, 15)
    for y in range(50, 62):
        half = 2 + (y - 50)
        fill_rect(stern, 32 - half, y, 32 - half + 1, y, 1)
        fill_rect(stern, 32 + half - 1, y, 32 + half, y, 1)
    pack_frame(3, stern)
    return out


def main():
    os.makedirs(BOOT, exist_ok=True)
    hulls = {name: paint_hull(*spec) for name, spec in KINDS.items()}
    zoom = bytearray()
    for name in ("escort", "merchant", "tanker", "uboat"):
        zoom += paint_small(*KINDS[name])
    files = {
        "escort.bin": hulls["escort"],
        "merchant.bin": hulls["merchant"],
        "tanker.bin": hulls["tanker"],
        "uboat.bin": hulls["uboat"],
        "wake.bin": paint_wake(),
        "marks.bin": paint_marks(),
        "zoom.bin": zoom,
        "profile.bin": (
            paint_profile(*KINDS["escort"])
            + paint_profile(*KINDS["merchant"])
            + paint_profile(*KINDS["tanker"])
            + paint_profile(*KINDS["uboat"])
        ),
    }
    for name, data in files.items():
        path = os.path.join(BOOT, name)
        open(path, "wb").write(data)
        print("wrote %s (%d)" % (name, len(data)))
    write_sea()
    # A north-aspect preview for the eye, not loaded by the game.
    try:
        from PIL import Image
    except ImportError:
        return
    pal = [
        (0, 0, 0), (255, 255, 255), (16, 32, 48), (160, 176, 192),
        (128, 96, 48), (192, 48, 32), (80, 144, 208), (112, 192, 240),
        (144, 224, 240), (64, 96, 80), (240, 224, 64), (128, 192, 240),
        (240, 32, 16), (96, 128, 160), (80, 80, 64), (0, 0, 32),
    ]
    sheet = Image.new("RGB", (64 * 4, 64))
    raw = files["escort.bin"]
    for frame in (0, 4, 8, 12):
        base = frame * FRAME
        for y in range(64):
            for x in range(0, 64, 2):
                b = raw[base + (y * 64 + x) // 2]
                sheet.putpixel((frame // 4 * 64 + x, y), pal[b >> 4])
                sheet.putpixel((frame // 4 * 64 + x + 1, y), pal[b & 15])
    sheet.save(os.path.join(BOOT, "aspects.png"))


if __name__ == "__main__":
    main()
