#!/usr/bin/env python3
"""把 chunks/*.omc 中的方块 id 压缩映射为一套"临时 id"，并导出映射表。

流程:
  1. 读取 exports/reports/blocks.json，建立 原始 id -> 方块名称(+属性) 的表；
  2. 扫描所有区块文件，统计出现过的方块 id；
  3. 按原始 id 升序分配临时 id（0,1,2,...），只分配给出现过的方块；
  4. 写出映射 JSON（临时 id <-> 方块名称），并把区块文件里的 id 替换为临时 id。

区块文件格式（version 2，展开式，little-endian）:
    magic char[4] "OMCH" | version u32 | chunkX i32 | chunkZ i32 | sectionCount u32
    per section: blockCount u16 | fluidCount u16 |
                 blockIds u32*4096 (索引 y*256+x*16+z) | biomeIds u32*64

用法:
    python3 scripts/map_chunk_ids.py                     # chunks/ -> chunks_mapped/
    python3 scripts/map_chunk_ids.py --in-place          # 直接覆盖 chunks/
    python3 scripts/map_chunk_ids.py --chunks chunks --out chunks_mapped
"""
import argparse
import glob
import json
import os
import struct
import sys

import numpy as np

SECTION_BLOCKS = 4096
SECTION_BIOMES = 64
HEADER = 20
SECTION_HEADER = 4
SECTION_SIZE = SECTION_HEADER + 4 * SECTION_BLOCKS + 4 * SECTION_BIOMES


def load_block_names(path):
    """原始 block state id -> 'minecraft:block[prop=value,...]'"""
    data = json.load(open(path, encoding="utf-8"))
    names = {}

    for block_name, entry in data.items():
        for state in entry.get("states", []):
            props = state.get("properties", {})
            if props:
                prop_str = ",".join(f"{k}={v}" for k, v in sorted(props.items()))
                names[state["id"]] = f"{block_name}[{prop_str}]"
            else:
                names[state["id"]] = f"{block_name}[]"

    return names


def read_header(buf):
    if buf[:4] != b"OMCH":
        raise ValueError("bad magic")
    version, cx, cz, nsec = struct.unpack_from("<IiiI", buf, 4)
    if version != 2:
        raise ValueError(f"unsupported version {version}")
    return cx, cz, nsec


def iter_block_ids(buf):
    """依次产出每个 section 的 blockIds numpy 视图"""
    cx, cz, nsec = read_header(buf)
    off = HEADER

    for _ in range(nsec):
        off += SECTION_HEADER
        ids = np.frombuffer(buf, dtype="<u4", count=SECTION_BLOCKS, offset=off)
        off += 4 * SECTION_BLOCKS
        off += 4 * SECTION_BIOMES
        yield ids


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--chunks", default="chunks")
    ap.add_argument("--out", default="chunks_mapped")
    ap.add_argument("--blocks", default="exports/reports/blocks.json")
    ap.add_argument("--mapping", default=None, help="映射 JSON 输出路径（默认 <out>/id_mapping.json）")
    ap.add_argument("--in-place", action="store_true", help="直接覆盖 chunks/ 下的文件")
    args = ap.parse_args()

    files = sorted(glob.glob(os.path.join(args.chunks, "*.omc")))

    if not files:
        sys.exit(f"no .omc files in {args.chunks}")

    names = load_block_names(args.blocks)

    # 1) 统计所有出现过的方块 id 及出现次数
    counts = {}

    for path in files:
        buf = open(path, "rb").read()

        for ids in iter_block_ids(buf):
            values, freqs = np.unique(ids, return_counts=True)

            for v, f in zip(values.tolist(), freqs.tolist()):
                counts[v] = counts.get(v, 0) + f

    print(f"files={len(files)} distinct blocks={len(counts)}")

    # 2) 分配临时 id（按原始 id 升序，保证可复现）
    temp_of_original = {orig: temp for temp, orig in enumerate(sorted(counts))}

    mapping = {
        "version": 1,
        "source": args.blocks,
        "tempIdToBlock": {},
        "blockToTempId": {},
        "blocks": [],
    }

    for orig in sorted(counts):
        temp = temp_of_original[orig]
        full = names.get(orig, f"<unknown id {orig}>")
        short = full.split("[", 1)[0]
        mapping["tempIdToBlock"][str(temp)] = full
        mapping["blockToTempId"][full] = temp
        mapping["blocks"].append(
            {"tempId": temp, "id": orig, "block": short, "name": full, "count": counts[orig]})

    out_dir = args.chunks if args.in_place else args.out
    os.makedirs(out_dir, exist_ok=True)

    mapping_path = args.mapping or os.path.join(out_dir, "id_mapping.json")

    with open(mapping_path, "w", encoding="utf-8") as f:
        json.dump(mapping, f, ensure_ascii=False, indent=2)

    print(f"mapping written: {mapping_path}")

    # 3) 生成查找表（原始 id -> 临时 id），用于向量化替换
    max_id = max(counts)
    lut = np.zeros(max_id + 1, dtype=np.uint32)

    for orig, temp in temp_of_original.items():
        lut[orig] = temp

    # 4) 重写区块文件
    for path in files:
        buf = bytearray(open(path, "rb").read())
        cx, cz, nsec = read_header(buf)
        off = HEADER

        for _ in range(nsec):
            off += SECTION_HEADER
            ids = np.frombuffer(buf, dtype="<u4", count=SECTION_BLOCKS, offset=off)
            mapped = lut[ids]
            buf[off:off + 4 * SECTION_BLOCKS] = mapped.astype("<u4").tobytes()
            off += 4 * SECTION_BLOCKS + 4 * SECTION_BIOMES

        target = path if args.in_place else os.path.join(args.out, os.path.basename(path))
        with open(target, "wb") as f:
            f.write(buf)

    print(f"rewritten chunks -> {out_dir}")


if __name__ == "__main__":
    main()
