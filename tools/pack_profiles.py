"""Shrink the painted 64px profile bins to 32, 16, and 8.

The bridge and the glasses share these. VERA cannot scale a sprite, so each
distance step is its own image. A pixel is the most common opaque nibble in
its block, which keeps a mast that nearest-neighbor would drop.
"""
import os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOOT = os.path.join(ROOT, "boot")
KINDS = ("escort", "merchant", "tanker", "uboat")
FILES = {"escort": "profesc.bin", "merchant": "profmer.bin", "tanker": "proftnk.bin", "uboat": "profubt.bin"}


def unpack(frame, size):
    px = [0] * (size * size)
    for i, b in enumerate(frame):
        px[2 * i] = b >> 4
        px[2 * i + 1] = b & 15
    return px


def pack(px):
    out = bytearray()
    for i in range(0, len(px), 2):
        out.append((px[i] << 4) | px[i + 1])
    return bytes(out)


def shrink(px, src, dst):
    step = src // dst
    out = [0] * (dst * dst)
    for y in range(dst):
        for x in range(dst):
            counts = {}
            for yy in range(step):
                for xx in range(step):
                    v = px[(y * step + yy) * src + x * step + xx]
                    if v:
                        counts[v] = counts.get(v, 0) + 1
            if counts:
                out[y * dst + x] = max(counts, key=counts.get)
    return out


def frames_of(path, size):
    raw = open(path, "rb").read()
    stride = (size * size) // 2
    n = len(raw) // stride
    return [unpack(raw[i * stride:(i + 1) * stride], size) for i in range(n)]


def main():
    kinds = []
    for name in KINDS:
        path = os.path.join(BOOT, FILES[name])
        got = frames_of(path, 64)
        if len(got) != 16:
            raise SystemExit("%s has %d frames" % (path, len(got)))
        kinds.append(got)
    small = {32: [], 16: [], 8: []}
    for got in kinds:
        for dst in (32, 16, 8):
            small[dst].append([shrink(fr, 64, dst) for fr in got])
    short = {"escort": "esc", "merchant": "mer", "tanker": "tnk", "uboat": "ubt"}
    for i, name in enumerate(KINDS):
        blob = b"".join(pack(fr) for fr in small[32][i])
        out = os.path.join(BOOT, "p32%s.bin" % short[name])
        open(out, "wb").write(blob)
        print("wrote %s (%d)" % (os.path.basename(out), len(blob)))
    for dst, name in ((16, "p16prof.bin"), (8, "p8prof.bin")):
        blob = b"".join(pack(fr) for kind in small[dst] for fr in kind)
        out = os.path.join(BOOT, name)
        open(out, "wb").write(blob)
        print("wrote %s (%d)" % (name, len(blob)))


if __name__ == "__main__":
    main()
