"""Write the Pixel and palette docs the game ships.

Run once when the art changes. The build does not call this. pixel-cli
exports the docs to boot\\*.bin, which is what the game loads.
"""
import json
import math
import os

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
PAL_PATH = os.path.join(ROOT, "assets", "palettes", "main.x16pal.json")

KINDS = {
    "escort": (50, 11, 3, 14, 0),
    "merchant": (46, 16, 4, 14, 0),
    "tanker": (52, 15, 4, 14, 5),
    "uboat": (48, 7, 9, 13, 0),
}


def load_colors():
    doc = json.load(open(PAL_PATH, encoding="utf-8"))
    return doc["colors"]


def hex_pixels(pix, w, h):
    s = []
    for y in range(h):
        for x in range(w):
            s.append("%x" % pix[y * w + x])
    return "".join(s)


def blank(w, h):
    return bytearray(w * h)


def put(pix, w, h, x, y, col):
    if col and 0 <= x < w and 0 <= y < h:
        pix[y * w + x] = col


def disc(pix, w, h, cx, cy, r, col):
    r2 = r * r
    for y in range(int(cy - r - 1), int(cy + r + 2)):
        for x in range(int(cx - r - 1), int(cx + r + 2)):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r2:
                put(pix, w, h, x, y, col)


def rot_local(x, y, ang):
    dx = x - 31.5
    dy = y - 31.5
    c = math.cos(ang)
    s = math.sin(ang)
    return dx * c + dy * s, dx * s - dy * c


def hull_color(lx, ly, length, beam, fill, deck, band, kind):
    if ly < -length / 2 or ly > length / 2:
        return 0
    t = (ly + length / 2) / length
    if t > 0.82:
        half = (beam / 2) * (1.0 - (t - 0.82) / 0.18)
    elif t < 0.10:
        half = (beam / 2) * (0.35 + 0.65 * (t / 0.10))
    else:
        half = beam / 2
    ax = abs(lx)
    if ax > half:
        return 0
    if ax > half - 1.3:
        return 2
    col = fill
    if band and 0.40 < t < 0.52 and ax < half - 2:
        col = band
    if ax < 1.3 and 0.16 < t < 0.88:
        col = deck
    # Fittings in ship space. Bow is +ly.
    if kind == "escort":
        if abs(lx) < 2.2 and 14 < ly < 20:
            col = 2
        if abs(lx) < 2.4 and -4 < ly < 6:
            col = 15
        if abs(lx) < 1.6 and -12 < ly < -6:
            col = 2
        if (lx * lx + (ly - 16) * (ly - 16)) < 4:
            col = 1
        if (lx * lx + (ly + 18) * (ly + 18)) < 3.5:
            col = 1
    elif kind == "merchant":
        if abs(lx) < 3 and 2 < ly < 10:
            col = 15
        if abs(lx) < 2 and -6 < ly < -1:
            col = 2
        if abs(ly - 8) < 2 and abs(lx) < half - 2:
            col = 14
    elif kind == "tanker":
        if abs(lx) < 1.2:
            col = 14
        if abs(lx) < 2.2 and -2 < ly < 4:
            col = 2
        if abs(ly) < 3 and abs(lx) < 3:
            col = 12
    elif kind == "uboat":
        if abs(lx) < 2.4 and -2 < ly < 8:
            col = 15
        if abs(lx) < 1.2 and 10 < ly < 16:
            col = 2
        if abs(lx) > half - 2.2 and -8 < ly < 4:
            col = 13
    return col


def paint_overhead(kind):
    length, beam, fill, deck, band = KINDS[kind]
    frames = []
    for frame in range(16):
        ang = frame * (math.pi / 8.0)
        pix = blank(64, 64)
        for y in range(64):
            for x in range(64):
                lx, ly = rot_local(x, y, ang)
                pix[y * 64 + x] = hull_color(lx, ly, length, beam, fill, deck, band, kind)
        frames.append(hex_pixels(pix, 64, 64))
    return frames


def paint_wake():
    frames = []
    for frame in range(16):
        ang = frame * (math.pi / 8.0)
        pix = blank(64, 64)
        for y in range(64):
            for x in range(64):
                lx, ly = rot_local(x, y, ang)
                if ly > -1 or ly < -46:
                    continue
                span = 1.1 + (-ly) * 0.08
                ax = abs(lx)
                if ax <= span:
                    pix[y * 64 + x] = 1 if ax > span - 0.7 else 11
        frames.append(hex_pixels(pix, 64, 64))
    return frames


def paint_scaled(kind, size):
    length, beam, fill, deck, band = KINDS[kind]
    step = 64.0 / size
    frames = []
    for frame in range(16):
        ang = frame * (math.pi / 8.0)
        pix = blank(size, size)
        for y in range(size):
            for x in range(size):
                lx, ly = rot_local(x * step + step * 0.5, y * step + step * 0.5, ang)
                pix[y * size + x] = hull_color(lx, ly, length, beam, fill, deck, band, kind)
        frames.append(hex_pixels(pix, size, size))
    return frames


def wake_color(lx, ly):
    if ly > -1 or ly < -46:
        return 0
    span = 1.1 + (-ly) * 0.08
    ax = abs(lx)
    if ax > span:
        return 0
    return 1 if ax > span - 0.7 else 11


def paint_wake_scaled(size):
    step = 64.0 / size
    frames = []
    for frame in range(16):
        ang = frame * (math.pi / 8.0)
        pix = blank(size, size)
        for y in range(size):
            for x in range(size):
                lx, ly = rot_local(x * step + step * 0.5, y * step + step * 0.5, ang)
                pix[y * size + x] = wake_color(lx, ly)
        frames.append(hex_pixels(pix, size, size))
    return frames


GLYPHS = [
    ["00111000", "00011000", "00001000", "00001000", "00001000", "00001000", "00111100", "00000000"],
    ["00111000", "01000100", "00000100", "00011000", "00100000", "01000000", "01111100", "00000000"],
    ["00111000", "01000100", "00000100", "00011000", "00000100", "01000100", "00111000", "00000000"],
    ["00001000", "00011000", "00101000", "01001000", "01111100", "00001000", "00001000", "00000000"],
]


def paint_digits(size):
    frames = []
    block = size // 8
    for rows in GLYPHS:
        pix = blank(size, size)
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch != "1":
                    continue
                for dy in range(block):
                    for dx in range(block):
                        put(pix, size, size, x * block + dx + block // 2, y * block + dy + block // 2, 2)
                        put(pix, size, size, x * block + dx, y * block + dy, 1)
        frames.append(hex_pixels(pix, size, size))
    return frames


def fill_rect(pix, x0, y0, x1, y1, col):
    if x0 > x1:
        x0, x1 = x1, x0
    if y0 > y1:
        y0, y1 = y1, y0
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            put(pix, 64, 64, x, y, col)


def profile_hit(kind, x, y, z, fill, deck, band):
    """Side-view solid. y is forward, x is across, z is up, water at 0."""
    if kind == "uboat":
        length, beam, top = 23.0, 3.0, 2.4
    elif kind == "tanker":
        length, beam, top = 25.0, 5.6, 4.2
    elif kind == "merchant":
        length, beam, top = 23.0, 6.2, 5.0
    else:
        length, beam, top = 24.0, 4.8, 4.6
    if y > length or y < -length:
        return 0
    taper = 1.0
    if y > length * 0.35:
        taper = max(0.0, (length - y) / (length * 0.65))
    elif kind == "uboat" and y < -length * 0.45:
        taper = max(0.0, (length + y) / (length * 0.55))
    if abs(x) > beam * taper:
        return 0
    if kind == "uboat":
        nx = x / max(beam * taper, 0.1)
        nz = (z - 0.4) / 2.6
        if nx * nx + nz * nz > 1.0 and not (abs(x) < 2.0 and 0.0 < y < 8.0 and 1.0 < z < 9.0):
            return 0
    if -3.2 <= z <= top:
        if z < -0.2:
            return 15
        if abs(z) < 0.7 and abs(x) > beam * taper * 0.55:
            return 1
        if band and top - 1.4 <= z <= top and y < length * 0.2:
            return band
        if z > top - 1.1:
            return deck
        return fill
    if kind == "escort":
        if abs(x) < 2.0 and 1.0 < y < 9.0 and top < z <= 12.0:
            return deck if z < 10.0 else 2
        if abs(x) < 1.6 and -8.0 < y < -3.0 and top < z <= 14.0:
            return 2
        if abs(x) < 0.6 and -1.0 < y < 2.0 and top < z <= 20.0:
            return 2
    elif kind == "merchant":
        if abs(x) < 3.2 and -2.0 < y < 8.0 and top < z <= 13.0:
            return deck if z < 11.0 else 2
        if abs(x) < 1.7 and -12.0 < y < -6.0 and top < z <= 15.0:
            return 2
        if abs(x) < 0.6 and 10.0 < y < 13.0 and top < z <= 18.0:
            return 2
    elif kind == "tanker":
        if abs(x) < 3.4 and -22.0 < y < -12.0 and top < z <= 15.0:
            return deck if z < 12.0 else 2
        if abs(x) < 2.4 and 14.0 < y < 22.0 and top < z <= 8.0:
            return deck
        if abs(x) < 0.7 and -16.0 < y < -14.0 and 8.0 < z <= 18.0:
            return 2
    elif kind == "uboat":
        if abs(x) < 1.8 and 1.0 < y < 8.0 and 2.0 < z <= 8.5:
            return deck if z < 7.0 else 15
        if abs(x) < 0.45 and 5.5 < y < 7.0 and 8.5 < z <= 15.0:
            return 2
    return 0


def paint_profiles(kind):
    """Sixteen aspects. Frame 0 is bow-on, then clockwise as seen from above."""
    _length, _beam, fill, deck, band = KINDS[kind]
    frames = []
    samples = []
    y = -26.0
    while y <= 26.0:
        x = -8.0
        while x <= 8.0:
            z = -4.0
            while z <= 22.0:
                col = profile_hit(kind, x, y, z, fill, deck, band)
                if col:
                    samples.append((x, y, z, col))
                z += 0.55
            x += 0.55
        y += 0.55
    for frame in range(16):
        ang = frame * math.pi / 8.0
        c = math.cos(ang)
        s = math.sin(ang)
        pix = blank(64, 64)
        depth = [-999.0] * (64 * 64)
        order = sorted(samples, key=lambda p: p[0] * s + p[1] * c)
        for x, y, z, col in order:
            sx = int(round(32.0 + (x * c - y * s)))
            sy = int(round(32.0 - z * 1.25))
            dep = x * s + y * c
            if 0 <= sx < 64 and 0 <= sy < 64:
                i = sy * 64 + sx
                if dep >= depth[i]:
                    depth[i] = dep
                    pix[i] = col
        for sy in range(64):
            for sx in range(64):
                if pix[sy * 64 + sx]:
                    continue
                hit = False
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        xx, yy = sx + dx, sy + dy
                        if 0 <= xx < 64 and 0 <= yy < 64 and pix[yy * 64 + xx] > 2:
                            hit = True
                if hit:
                    pix[sy * 64 + sx] = 2
        frames.append(hex_pixels(pix, 64, 64))
    return frames


def paint_dial():
    pix = blank(64, 64)
    disc(pix, 64, 64, 31.5, 31.5, 30, 2)
    disc(pix, 64, 64, 31.5, 31.5, 26, 15)
    disc(pix, 64, 64, 31.5, 31.5, 22, 14)
    for i in range(16):
        a = i * math.pi / 8
        x = int(31.5 + math.sin(a) * 24)
        y = int(31.5 - math.cos(a) * 24)
        put(pix, 64, 64, x, y, 1)
        put(pix, 64, 64, x + 1, y, 1)
    return [hex_pixels(pix, 64, 64)]


def paint_beam():
    frames = []
    for i in range(16):
        ang = i * math.pi / 8
        pix = blank(16, 16)
        for t in range(0, 8):
            x = int(7.5 + math.sin(ang) * t)
            y = int(7.5 - math.cos(ang) * t)
            put(pix, 16, 16, x, y, 12)
            put(pix, 16, 16, x + 1, y, 1)
        frames.append(hex_pixels(pix, 16, 16))
    return frames


def paint_foam():
    pix = blank(16, 16)
    disc(pix, 16, 16, 7.5, 7.5, 6, 1)
    disc(pix, 16, 16, 7.5, 7.5, 2, 15)
    return [hex_pixels(pix, 16, 16)]


def paint_ring():
    pix = blank(32, 32)
    y = 0
    while y < 32:
        x = 0
        while x < 32:
            d = math.hypot(x - 15.5, y - 15.5)
            if 9.5 <= d <= 14.5:
                pix[y * 32 + x] = 1 if d >= 12.5 else 15
            x += 1
        y += 1
    return [hex_pixels(pix, 32, 32)]


def paint_blip(col):
    pix = blank(16, 16)
    disc(pix, 16, 16, 7.5, 7.5, 4, col)
    return [hex_pixels(pix, 16, 16)]


def paint_face():
    """Top-left quadrant of a 128px scope. The circle's center is the bottom-right corner."""
    pix = blank(64, 64)
    cx, cy = 63.5, 63.5
    for y in range(64):
        for x in range(64):
            dx = x - cx
            dy = y - cy
            r2 = dx * dx + dy * dy
            if r2 > 60.5 * 60.5:
                continue
            r = r2 ** 0.5
            if r >= 56:
                pix[y * 64 + x] = 2
            elif abs(r - 38) < 1.1 or abs(r - 20) < 1.0:
                pix[y * 64 + x] = 15
            else:
                pix[y * 64 + x] = 14
    for i in range(32):
        a = i * math.pi / 16
        put(pix, 64, 64, int(cx + math.sin(a) * 58), int(cy - math.cos(a) * 58), 1)
    return [hex_pixels(pix, 64, 64)]


def paint_needles():
    frames = []
    for i in range(8):
        ang = i * math.pi / 4
        pix = blank(32, 32)
        for t in range(2, 14):
            x = int(15.5 + math.sin(ang) * t)
            y = int(15.5 - math.cos(ang) * t)
            put(pix, 32, 32, x, y, 12)
            put(pix, 32, 32, x + 1, y, 12)
        disc(pix, 32, 32, 15.5, 15.5, 2, 1)
        frames.append(hex_pixels(pix, 32, 32))
    return frames


def paint_eel_rot():
    frames = []
    for i in range(16):
        ang = i * math.pi / 8
        pix = blank(16, 16)
        for t in range(-6, 6):
            x = int(7.5 + math.sin(ang) * t * 0.7)
            y = int(7.5 - math.cos(ang) * t * 0.7)
            col = 1 if t > 3 else 10
            put(pix, 16, 16, x, y, col)
            put(pix, 16, 16, x + 1, y, col)
        frames.append(hex_pixels(pix, 16, 16))
    return frames


def paint_twake():
    frames = []
    for i in range(16):
        ang = i * math.pi / 8
        pix = blank(16, 16)
        for t in range(0, 7):
            x = int(7.5 - math.sin(ang) * t)
            y = int(7.5 + math.cos(ang) * t)
            put(pix, 16, 16, x, y, 1 if t < 2 else 11)
        frames.append(hex_pixels(pix, 16, 16))
    return frames


def paint_charge_body():
    frames = []
    for i in range(16):
        ang = i * math.pi / 8
        pix = blank(16, 16)
        for t in range(-4, 4):
            x = int(7.5 + math.sin(ang) * t * 0.6)
            y = int(7.5 - math.cos(ang) * t * 0.6)
            put(pix, 16, 16, x, y, 15)
            put(pix, 16, 16, x + 1, y, 2)
        frames.append(hex_pixels(pix, 16, 16))
    return frames


def paint_splash_rot():
    frames = []
    for i in range(16):
        ang = i * math.pi / 8
        pix = blank(16, 16)
        disc(pix, 16, 16, 7.5, 7.5, 5, 11)
        for t in range(-5, 6):
            x = int(7.5 + math.sin(ang) * t)
            y = int(7.5 - math.cos(ang) * t * 0.45)
            put(pix, 16, 16, x, y, 1)
        frames.append(hex_pixels(pix, 16, 16))
    return frames


def paint_column():
    pix = blank(32, 32)
    for y in range(4, 28):
        half = 3 + (28 - y) // 6
        for x in range(16 - half, 16 + half):
            pix[y * 32 + x] = 1 if y < 10 else 11
    disc(pix, 32, 32, 15.5, 26, 8, 1)
    return [hex_pixels(pix, 32, 32)]


def paint_lamp(col):
    pix = blank(16, 16)
    disc(pix, 16, 16, 7.5, 7.5, 6, 2)
    disc(pix, 16, 16, 7.5, 7.5, 4, col)
    return [hex_pixels(pix, 16, 16)]


def paint_tube(full):
    pix = blank(16, 16)
    for y in range(4, 12):
        for x in range(2, 14):
            pix[y * 16 + x] = 2
    for y in range(6, 10):
        for x in range(4, 12):
            pix[y * 16 + x] = 10 if full else 15
    if full:
        pix[7 * 16 + 11] = 1
        pix[8 * 16 + 11] = 1
    return [hex_pixels(pix, 16, 16)]


def paint_cross():
    pix = blank(64, 64)
    for i in range(64):
        if 4 < abs(i - 32) < 30:
            pix[32 * 64 + i] = 1
            pix[i * 64 + 32] = 1
        if abs(i - 32) in (8, 16, 24):
            for d in range(-2, 3):
                put(pix, 64, 64, i, 32 + d, 10)
                put(pix, 64, 64, 32 + d, i, 10)
    return [hex_pixels(pix, 64, 64)]


def paint_eel_burst():
    eel = blank(16, 16)
    burst = blank(16, 16)
    for y in range(16):
        for x in range(16):
            if 7 <= x <= 8 and 2 <= y <= 13:
                eel[y * 16 + x] = 10
            if 6 <= y <= 9 and 3 <= x <= 12:
                eel[y * 16 + x] = 1 if x < 5 else 10
            # Last-known plot mark. A small diamond, not a detonation.
            if abs(x - 7) + abs(y - 7) <= 2:
                burst[y * 16 + x] = 12
    return [hex_pixels(eel, 16, 16), hex_pixels(burst, 16, 16)]


def sheet(sid, name, size, frames):
    return {
        "id": sid,
        "name": name,
        "size": size,
        "width": size,
        "height": size,
        "compose": "single",
        "paletteOffset": 0,
        "layoutLocked": False,
        "pixelWidth": size,
        "pixelHeight": size,
        "frames": [{"name": "%s-%d" % (sid, i), "pixels": p} for i, p in enumerate(frames)],
    }


def write_sprites(path, resource, sheets):
    colors = load_colors()
    doc = {
        "format": "x16.sprites",
        "schema": 1,
        "project": "../../x16project.json",
        "resource": resource,
        "palette": "main",
        "protocolVersion": 1,
        "sessionId": "convoy-" + resource,
        "host": {
            "id": "standalone",
            "displayName": "Standalone",
            "instanceId": "convoy",
            "projectPath": ROOT,
            "projectName": "convoy-x16",
        },
        "sprites": {"version": 1, "palette": colors, "sheets": sheets},
    }
    os.makedirs(os.path.dirname(path), exist_ok=True)
    json.dump(doc, open(path, "w", encoding="utf-8"), indent=2)
    print("wrote", path)


def write_sea():
    def row(parts):
        line = "".join(parts)
        if len(line) != 16:
            raise SystemExit("row %d %s" % (len(line), line))
        return line

    solid = "6" * 256
    wave = (
        "6" * 16 * 4
        + row("66668bbbbbbb8666")
        + row("6666bb1111bb8666")
        + row("66668bbbbbbb6666")
        + "6" * 16 * 9
    )
    chop = (
        "6" * 16 * 10
        + row("666666668b866666")
        + row("66666666b1186666")
        + row("666666668b866666")
        + "6" * 16 * 3
    )
    foam = (
        "6" * 16 * 6
        + row("6666661111666666")
        + row("6666618888166666")
        + row("6666661111666666")
        + "6" * 16 * 7
    )
    path = os.path.join(ROOT, "assets", "tiles", "sea.x16tile.json")
    doc = json.load(open(path, encoding="utf-8"))
    named = {
        "deep": solid,
        "mid": wave,
        "chop": chop,
        "foam": foam,
        "sky": "8" * 256,
        "horizon": "8" * 224 + "7" * 16 + "1" * 16,
    }
    for tile in doc["tiles"]:
        tile["pixels"] = named[tile["name"]]
    json.dump(doc, open(path, "w", encoding="utf-8"), indent=2)
    print("wrote", path)


def main():
    spr = os.path.join(ROOT, "assets", "sprites")
    write_sprites(
        os.path.join(spr, "hulls.x16spr.json"),
        "hulls",
        [
            sheet("escort", "Escort", 64, paint_overhead("escort")),
            sheet("merchant", "Merchant", 64, paint_overhead("merchant")),
            sheet("tanker", "Tanker", 64, paint_overhead("tanker")),
            sheet("uboat", "U-boat", 64, paint_overhead("uboat")),
            sheet("wake", "Wake", 64, paint_wake()),
        ],
    )
    write_sprites(
        os.path.join(spr, "profiles.x16spr.json"),
        "profiles",
        [sheet(name, name, 64, paint_profiles(name)) for name in ("escort", "merchant", "tanker", "uboat")],
    )
    write_sprites(
        os.path.join(spr, "zoom.x16spr.json"),
        "zoom",
        [sheet(name, name, 8, paint_scaled(name, 8)) for name in ("escort", "merchant", "tanker", "uboat")]
        + [sheet("wake", "Wake", 8, paint_wake_scaled(8))],
    )
    eel, burst = paint_eel_burst()
    write_sprites(
        os.path.join(spr, "marks.x16spr.json"),
        "marks",
        [
            sheet("eel", "Torpedo", 16, [eel]),
            sheet("burst", "Burst", 16, [burst]),
            sheet("cross", "Periscope cross", 64, paint_cross()),
        ],
    )
    write_sprites(
        os.path.join(spr, "gauges.x16spr.json"),
        "gauges",
        [
            sheet("dial", "Dial", 64, paint_dial()),
            sheet("needle", "Needle", 32, paint_needles()),
            sheet("lamp-on", "Lamp on", 16, paint_lamp(10)),
            sheet("lamp-off", "Lamp off", 16, paint_lamp(15)),
            sheet("tube-full", "Tube loaded", 16, paint_tube(True)),
            sheet("tube-empty", "Tube spent", 16, paint_tube(False)),
        ],
    )
    write_sprites(
        os.path.join(spr, "mid.x16spr.json"),
        "mid",
        [sheet(name, name, 32, paint_scaled(name, 32)) for name in ("escort", "merchant", "tanker", "uboat")]
        + [sheet("wake", "Wake", 32, paint_wake_scaled(32))],
    )
    write_sprites(
        os.path.join(spr, "tiny.x16spr.json"),
        "tiny",
        [sheet(name, name, 16, paint_scaled(name, 16)) for name in ("escort", "merchant", "tanker", "uboat")]
        + [sheet("wake", "Wake", 16, paint_wake_scaled(16))],
    )
    write_sprites(
        os.path.join(spr, "charge.x16spr.json"),
        "charge",
        [
            sheet("body", "Depth charge", 16, paint_charge_body()),
            sheet("splash", "Splash", 16, paint_splash_rot()),
        ],
    )
    write_sprites(
        os.path.join(spr, "numbers.x16spr.json"),
        "numbers",
        [
            sheet("num16", "Escort number", 16, paint_digits(16)),
            sheet("num8", "Escort number small", 8, paint_digits(8)),
        ],
    )
    write_sprites(
        os.path.join(spr, "scope.x16spr.json"),
        "scope",
        [
            sheet("beam", "Radar sweep", 16, paint_beam()),
            sheet("face", "Scope quadrant", 64, paint_face()),
            sheet("blip", "Friendly blip", 16, paint_blip(1)),
            sheet("blip-red", "Unknown blip", 16, paint_blip(12)),
            sheet("foam", "Wake foam", 16, paint_foam()),
            sheet("ring", "Wake ring", 32, paint_ring()),
        ],
    )
    write_sprites(
        os.path.join(spr, "weapons.x16spr.json"),
        "weapons",
        [
            sheet("eel", "Torpedo", 16, paint_eel_rot()),
            sheet("twake", "Torpedo wake", 16, paint_twake()),
            sheet("column", "Water column", 32, paint_column()),
        ],
    )
    write_sea()


if __name__ == "__main__":
    main()
