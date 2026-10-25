#!/usr/bin/env python3
"""Generate src/openminecraft/data/biome/om_biome_registry.cpp from a datapack export.

Usage:
    python3 scripts/gen_biome_registry.py [--datapack DIR] [--colormap DIR] [--output FILE]

Colors are baked at generation time (see the header comment in the output).
"""
import argparse
import glob
import json
import os
import struct
import zlib


def decode_png(path):
    data = open(path, "rb").read()
    w, h = struct.unpack(">II", data[16:24])
    ctype = data[25]
    idat = b""
    i = 8
    while i < len(data):
        ln = struct.unpack(">I", data[i:i + 4])[0]
        typ = data[i + 4:i + 8]
        if typ == b"IDAT":
            idat += data[i + 8:i + 8 + ln]
        i += 12 + ln
    raw = zlib.decompress(idat)
    ch = {2: 3, 6: 4, 0: 1}[ctype]
    stride = w * ch
    out = bytearray()
    prev = bytearray(stride)
    pos = 0
    for _ in range(h):
        ft = raw[pos]
        pos += 1
        line = bytearray(raw[pos:pos + stride])
        pos += stride
        if ft == 1:
            for x in range(ch, stride):
                line[x] = (line[x] + line[x - ch]) & 0xFF
        elif ft == 2:
            for x in range(stride):
                line[x] = (line[x] + prev[x]) & 0xFF
        elif ft == 3:
            for x in range(stride):
                a = line[x - ch] if x >= ch else 0
                line[x] = (line[x] + ((a + prev[x]) >> 1)) & 0xFF
        elif ft == 4:
            for x in range(stride):
                a = line[x - ch] if x >= ch else 0
                b = prev[x]
                c = prev[x - ch] if x >= ch else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[x] = (line[x] + pr) & 0xFF
        out += line
        prev = line
    return w, h, ch, out


def colormap(decoded, temperature, downfall):
    w, h, ch, px = decoded
    humidity = temperature * downfall
    x = int((1.0 - temperature) * 255.0)
    y = int((1.0 - humidity) * 255.0)
    x = max(0, min(255, x))
    y = max(0, min(255, y))
    i = (y * w + x) * ch
    return (px[i] << 16) | (px[i + 1] << 8) | px[i + 2]


def hexcol(value):
    return int(value[1:], 16)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--datapack", default="exports/datapack/minecraft/worldgen/biome")
    parser.add_argument("--colormap", default="externalassets/assets/minecraft/textures/colormap")
    parser.add_argument("--output", default="src/openminecraft/data/biome/om_biome_registry.cpp")
    args = parser.parse_args()

    cm = {n: decode_png(os.path.join(args.colormap, n + ".png")) for n in ("grass", "foliage", "dry_foliage")}

    lines = []
    for path in sorted(glob.glob(os.path.join(args.datapack, "*.json"))):
        b = json.load(open(path))
        name = os.path.basename(path)[:-5]
        temp = float(b["temperature"])
        downfall = float(b["downfall"])
        eff = b.get("effects", {})
        attrs = b.get("attributes", {})

        grass = colormap(cm["grass"], temp, downfall)
        if "grass_color" in eff:
            grass = hexcol(eff["grass_color"])
        modifier = eff.get("grass_color_modifier")
        if modifier == "swamp":
            grass = 0x6A7039
        elif modifier == "dark_forest":
            grass = (grass & 0xFEFEFE) >> 1

        foliage = hexcol(eff["foliage_color"]) if "foliage_color" in eff else colormap(cm["foliage"], temp, downfall)
        dry = hexcol(eff["dry_foliage_color"]) if "dry_foliage_color" in eff else colormap(cm["dry_foliage"], temp, downfall)
        water = hexcol(eff.get("water_color", "#3F76E4"))
        sky = hexcol(attrs.get("minecraft:visual/sky_color", "#78A7FF"))
        fog = hexcol(attrs.get("minecraft:visual/fog_color", "#C0D8FF"))

        lines.append(
            f'    biomeRegistry.registerItem(OMIdentifier("minecraft:{name}"), std::make_shared<OMBiome>()\n'
            f"        ->setTemperature({temp}f)\n"
            f"        ->setDownfall({downfall}f)\n"
            f"        ->setGrassColor(0x{grass:06X})\n"
            f"        ->setWaterColor(0x{water:06X})\n"
            f"        ->setFoliageColor(0x{foliage:06X})\n"
            f"        ->setDryFoliageColor(0x{dry:06X})\n"
            f"        ->setSkyColor(0x{sky:06X})\n"
            f"        ->setFogColor(0x{fog:06X}));\n"
        )

    header = '''#include "openminecraft-shell/data/biome/om_biome_registry.hpp"
#include "openminecraft-shell/data/biome/om_biome.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include <memory>

// INFO: generated from a datapack export by scripts/gen_biome_registry.py.
// All colors are baked at generation time:
//   - grass / foliage / dry_foliage: effects override, otherwise sampled from
//     textures/colormap/{grass,foliage,dry_foliage}.png at (temperature, downfall)
//   - grass_color_modifier: swamp -> 0x6A7039, dark_forest -> (c & 0xFEFEFE) >> 1
//   - water_color: effects.water_color
//   - sky_color: attributes[ minecraft:visual/sky_color ]
//   - fog_color: attributes[ minecraft:visual/fog_color ], default 0xC0D8FF
namespace openminecraft::shell::data::biome
{
world::OMWorldRegistry<OMIdentifier, std::shared_ptr<OMBiome>> &biomeRegistry =
    *new world::OMWorldRegistry<OMIdentifier, std::shared_ptr<OMBiome>>;

void registerBiomes()
{
'''

    footer = "}\n} // namespace openminecraft::shell::data::biome\n"

    with open(args.output, "w") as f:
        f.write(header + "".join(lines) + footer)

    print(f"registered {len(lines)} biomes -> {args.output}")


if __name__ == "__main__":
    main()
