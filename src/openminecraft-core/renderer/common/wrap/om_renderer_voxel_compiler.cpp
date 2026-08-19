#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include <array>
#include <cstdint>

namespace openminecraft::renderer::common::wrap
{
OMVoxelCompiler::OMVoxelCompiler() : logger("OMVoxelCompiler", this)
{
}
OMVoxelCompiler::~OMVoxelCompiler()
{
}

// INFO: letter -> meanings
// x -> Voxel X Coordinate (4 bits)
// y -> Voxel Y Coordinate (4 bits)
// z -> Voxel Z Coordinate (4 bits)
// X -> Voxel X Coordinate Div (4 bits)
// Y -> Voxel Y Coordinate Div (4 bits)
// Z -> Voxel Z Coordinate Div (4 bits)
// S -> Voxel Div Sign (3 bits)
// s -> Voxel Scale (3 * 5 bits)
// e -> Voxel Enable (1 bit)
// a -> Voxel Ambient Occclusion Levels (4 * 2 bits)
// t -> Voxel Texture Index (16 bits)
// c -> Voxel Chunk ID (16 bits)
// r -> Voxel Rotation (2 bits, 00 -> 0deg, 01 -> 90deg, 10 -> 180deg, 11 -> 270deg)
// l -> Voxel Sky Light (4 * 4 bits)
// L -> Voxel Block Light (4 * 4 bits)
// U -> Voxel UV Offset (4 * 5 bits)
// A -> Voxel Rotation Axis (2 bits)
// C -> Voxel Rotation Center (3 * 5 bits)
// n -> Voxel Angle (3 bits)
// G -> Voxel Uses Generated UV (1 bit)
// u -> unused
// INFO: packed vertex structure in u32
// xxxx yyyy zzzz efff XXXX YYYY ZZZZ SSSU
// rrtt tttt tttt tttt cccc cccc cccc cccc
// llll llll llll llll LLLL LLLL LLLL LLLL
// sssss sssss sssss U UUUU UUUU aaaa aaaa
// UUUUU UUUUU AA CCC CCCC CCCC CCCC uGnnn
static constexpr auto packVoxel(uint8_t x, uint8_t y, uint8_t z, OMVoxelFacing facing, uint8_t dx, uint8_t dy,
                                uint8_t dz, bool negx, bool negy, bool negz, uint16_t tex, uint16_t chkid,
                                uint8_t rotation, uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t bl1,
                                uint8_t bl2, uint8_t bl3, uint8_t bl4, uint8_t scaleX, uint8_t scaleY, uint8_t scaleZ,
                                uint8_t ao1, uint8_t ao2, uint8_t ao3, uint8_t ao4, uint8_t u0, uint8_t v0, uint8_t u1,
                                uint8_t v1, uint8_t rotationAxis, uint8_t rotationCx, uint8_t rotationCy,
                                uint8_t rotationCz, bool rCxNeg, bool rCyNeg, bool rCzNeg, uint8_t rAngle, bool genUV)
    -> std::array<int, 5>
{
    return {
        x << 28 | y << 24 | z << 20 | 1 << 19 | (facing & 7) << 16 | dx << 12 | dy << 8 | dz << 4 | ((negx & 1) << 3) |
            ((negy & 1) << 2) | ((negz & 1) << 1) | ((u0 >> 4) & 1),
        (rotation << 30) | ((tex & 0x3fff) << 16) | chkid,
        ((l1 & 0xf) << 28) | ((l2 & 0xf) << 24) | ((l3 & 0xf) << 20) | ((l4 & 0xf) << 16) | ((bl1 & 0xf) << 12) |
            ((bl2 & 0xf) << 8) | ((bl3 & 0xf) << 4) | (bl4 & 0xf),
        ((scaleX & 0x1f) << 27) | ((scaleY & 0x1f) << 22) | ((scaleZ & 0x1f) << 17) | ao1 << 6 | ao2 << 4 | ao3 << 2 |
            ao4 | ((v0 & 0x1f) << 12) | ((u0 & 0xf) << 8),
        ((u1 & 0x1f) << 27) | ((v1 & 0x1f) << 22) | ((rotationAxis & 3) << 20) | (rCxNeg << 19) | (rCyNeg << 18) |
            (rCzNeg << 17) | ((rotationCx & 0xf) << 13) | ((rotationCy & 0xf) << 9) | ((rotationCz & 0xf) << 5) |
            (genUV << 3) | (rAngle & 7),
    };
}

auto OMVoxelCompiler::compile(const world::OMChunk<16> &chunk,
                              std::function<bool(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor, int chunkid,
                              std::function<void(OMVoxel)> commiter) -> void
{
    auto exist = [&](int x, int y, int z) -> bool {
        if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
            return externalAccessor(glm::ivec3(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz);
        return chunk.exists(x, y, z);
    };

    auto computeAO = [&](int x, int y, int z, OMVoxelFacing facing) -> std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> {
        uint8_t ao1 = 0, ao2 = 0, ao3 = 0, ao4 = 0;

        auto countNeighbors = [&](int dx, int dy, int dz) -> int { return exist(x + dx, y + dy, z + dz) ? 1 : 0; };
        auto finalAO = [&](int corner, int side1, int side2) {
            if (side1 && side2)
            {
                return 3;
            }
            return side1 + side2 + corner;
        };

        switch (facing)
        {
        case OMVoxelFacing::NegY: {
            ao1 = finalAO(countNeighbors(-1, -1, -1), countNeighbors(-1, -1, 0), countNeighbors(0, -1, -1));
            ao2 = finalAO(countNeighbors(-1, -1, 1), countNeighbors(-1, -1, 0), countNeighbors(0, -1, 1));
            ao3 = finalAO(countNeighbors(1, -1, -1), countNeighbors(1, -1, 0), countNeighbors(0, -1, -1));
            ao4 = finalAO(countNeighbors(1, -1, 1), countNeighbors(1, -1, 0), countNeighbors(0, -1, 1));
            break;
        }
        case OMVoxelFacing::PosY: {
            ao1 = finalAO(countNeighbors(-1, 1, -1), countNeighbors(-1, 1, 0), countNeighbors(0, 1, -1));
            ao2 = finalAO(countNeighbors(-1, 1, 1), countNeighbors(-1, 1, 0), countNeighbors(0, 1, 1));
            ao3 = finalAO(countNeighbors(1, 1, -1), countNeighbors(1, 1, 0), countNeighbors(0, 1, -1));
            ao4 = finalAO(countNeighbors(1, 1, 1), countNeighbors(1, 1, 0), countNeighbors(0, 1, 1));
            break;
        }
        case OMVoxelFacing::NegX: {
            ao2 = finalAO(countNeighbors(-1, -1, -1), countNeighbors(-1, -1, 0), countNeighbors(-1, 0, -1));
            ao1 = finalAO(countNeighbors(-1, 1, -1), countNeighbors(-1, 1, 0), countNeighbors(-1, 0, -1));
            ao4 = finalAO(countNeighbors(-1, -1, 1), countNeighbors(-1, -1, 0), countNeighbors(-1, 0, 1));
            ao3 = finalAO(countNeighbors(-1, 1, 1), countNeighbors(-1, 1, 0), countNeighbors(-1, 0, 1));
            break;
        }
        case OMVoxelFacing::PosX: {
            ao4 = finalAO(countNeighbors(1, -1, -1), countNeighbors(1, -1, 0), countNeighbors(1, 0, -1));
            ao3 = finalAO(countNeighbors(1, 1, -1), countNeighbors(1, 1, 0), countNeighbors(1, 0, -1));
            ao2 = finalAO(countNeighbors(1, -1, 1), countNeighbors(1, -1, 0), countNeighbors(1, 0, 1));
            ao1 = finalAO(countNeighbors(1, 1, 1), countNeighbors(1, 1, 0), countNeighbors(1, 0, 1));
            break;
        }
        case OMVoxelFacing::NegZ: {
            ao4 = finalAO(countNeighbors(-1, -1, -1), countNeighbors(-1, 0, -1), countNeighbors(0, -1, -1));
            ao3 = finalAO(countNeighbors(-1, 1, -1), countNeighbors(-1, 0, -1), countNeighbors(0, 1, -1));
            ao2 = finalAO(countNeighbors(1, -1, -1), countNeighbors(1, 0, -1), countNeighbors(0, -1, -1));
            ao1 = finalAO(countNeighbors(1, 1, -1), countNeighbors(1, 0, -1), countNeighbors(0, 1, -1));
            break;
        }
        case OMVoxelFacing::PosZ: {
            ao2 = finalAO(countNeighbors(-1, -1, 1), countNeighbors(-1, 0, 1), countNeighbors(0, -1, 1));
            ao1 = finalAO(countNeighbors(-1, 1, 1), countNeighbors(-1, 0, 1), countNeighbors(0, 1, 1));
            ao4 = finalAO(countNeighbors(1, -1, 1), countNeighbors(1, 0, 1), countNeighbors(0, -1, 1));
            ao3 = finalAO(countNeighbors(1, 1, 1), countNeighbors(1, 0, 1), countNeighbors(0, 1, 1));
            break;
        }
        default:
            break;
        }
        return {ao1, ao2, ao3, ao4};
    };

    for (const auto &v : chunk)
    {
        if (handler->queryNumParts(v.second) == 0)
        {
            continue;
        }

        auto checkExists = [&](OMVoxelFacing f) -> bool {
            switch (f)
            {
            case None:
                return false;
            case NegX:
                return exist(v.first.x - 1, v.first.y, v.first.z);
            case NegY:
                return exist(v.first.x, v.first.y - 1, v.first.z);
            case NegZ:
                return exist(v.first.x, v.first.y, v.first.z - 1);
            case PosX:
                return exist(v.first.x + 1, v.first.y, v.first.z);
            case PosY:
                return exist(v.first.x, v.first.y + 1, v.first.z);
            case PosZ:
                return exist(v.first.x, v.first.y, v.first.z + 1);
            }
        };

        for (int i = 0; i < handler->queryNumParts(v.second); ++i)
        {
            for (auto f : {NegX, NegY, NegZ, PosX, PosY, PosZ})
            {
                if (handler->queryPartFaceEnabled(v.second, i, f) &&
                    !checkExists(handler->queryPartFaceCull(v.second, i, f)))
                {
                    auto [ao1, ao2, ao3, ao4] = computeAO(v.first.x, v.first.y, v.first.z, f);
                    auto vox =
                        packVoxel(v.first.x, v.first.y, v.first.z, f, 0, 0, 0, true, true, true,
                                  handler->queryPartFaceTex(v.second, i, f), chunkid, 0, 15, 15, 15, 15, 0, 0, 0, 0, 16,
                                  16, 16, ao1, ao2, ao3, ao4, 0, 0, 16, 16, 0, 0, 0, 0, false, false, false, 2, false);
                    commiter(OMVoxel{vox[0], vox[1], vox[2], vox[3], vox[4]});
                }
            }
        }
    }
}
} // namespace openminecraft::renderer::common::wrap