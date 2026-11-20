#!/usr/bin/env python3
"""读取 headless client 保存的区块文件（./chunks/*.omc，格式版本 2，展开形式）。

文件格式（little-endian）:
    magic          char[4]  "OMCH"
    version        u32      2
    chunkX         i32
    chunkZ         i32
    sectionCount   u32
    每个 section:
        blockCount u16
        fluidCount u16
        blockIds   u32 * 4096    # 索引 = y*256 + z*16 + x（原版顺序），值为原始 block state id
        biomeIds   u32 * 64      # 索引 = y*16 + x*4 + z

用法:
    python3 scripts/read_chunks.py chunks/0_0.omc
    python3 scripts/read_chunks.py chunks/
"""
import glob
import os
import struct
import sys
from collections import Counter

SECTION_BLOCKS = 4096
SECTION_BIOMES = 64


def read_chunk(path):
    buf = open(path, "rb").read()

    if buf[:4] != b"OMCH":
        raise ValueError(f"{path}: bad magic")

    version, cx, cz, nsec = struct.unpack_from("<IiiI", buf, 4)
    if version != 2:
        raise ValueError(f"{path}: unsupported version {version}")

    off = 20
    sections = []

    for _ in range(nsec):
        block_count, fluid_count = struct.unpack_from("<HH", buf, off)
        off += 4
        blocks = struct.unpack_from(f"<{SECTION_BLOCKS}I", buf, off)
        off += 4 * SECTION_BLOCKS
        biomes = struct.unpack_from(f"<{SECTION_BIOMES}I", buf, off)
        off += 4 * SECTION_BIOMES
        sections.append({"blockCount": block_count, "fluidCount": fluid_count, "blocks": blocks, "biomes": biomes})

    return {"version": version, "x": cx, "z": cz, "sections": sections, "size": len(buf)}


def summarize(path, verbose=False):
    c = read_chunk(path)
    print(f"{os.path.basename(path)}: chunk=({c['x']},{c['z']}) sections={len(c['sections'])} "
          f"size={c['size']} bytes")

    for i, s in enumerate(c["sections"]):
        counter = Counter(s["blocks"])
        top = counter.most_common(4)
        print(f"  section[{i}] blockCount={s['blockCount']} fluidCount={s['fluidCount']} "
              f"unique={len(counter)} top={top}")

        if verbose:
            print("    first 16:", s["blocks"][:16])


if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "chunks"
    verbose = "--verbose" in sys.argv
    files = sorted(glob.glob(os.path.join(target, "*.omc"))) if os.path.isdir(target) else [target]

    if not files:
        sys.exit(f"no .omc files in {target}")

    for f in files[:20]:
        summarize(f, verbose)

    if len(files) > 20:
        print(f"... {len(files)} files total")
