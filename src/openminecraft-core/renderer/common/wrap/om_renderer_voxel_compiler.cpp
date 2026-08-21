#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"
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
// u -> unused
// INFO: packed vertex structure in u32
// xxxx yyyy zzzz efff XXXX YYYY ZZZZ SSSU
// rrtt tttt tttt tttt cccc cccc cccc cccc
// llll llll llll llll LLLL LLLL LLLL LLLL
// sssss sssss sssss U UUUU UUUU aaaa aaaa
// UUUUU UUUUU AA CCC CCCC CCCC CCCC uunnn
static constexpr auto packVoxel(uint8_t x, uint8_t y, uint8_t z, OMVoxelFacing facing, uint8_t dx, uint8_t dy,
                                uint8_t dz, bool negx, bool negy, bool negz, uint16_t tex, uint16_t chkid,
                                uint8_t rotation, uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t bl1,
                                uint8_t bl2, uint8_t bl3, uint8_t bl4, uint8_t scaleX, uint8_t scaleY, uint8_t scaleZ,
                                uint8_t ao1, uint8_t ao2, uint8_t ao3, uint8_t ao4, uint8_t u0, uint8_t v0, uint8_t u1,
                                uint8_t v1, uint8_t rotationAxis, uint8_t rotationCx, uint8_t rotationCy,
                                uint8_t rotationCz, bool rCxNeg, bool rCyNeg, bool rCzNeg, uint8_t rAngle)
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
            (rAngle & 7),
    };
}

auto OMVoxelCompiler::existSoild(const world::OMChunk<16> &chunk,
                                 std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor, int x,
                                 int y, int z) -> bool
{
    if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
        return handler->querySoild(externalAccessor(glm::ivec3(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz));

    return handler->querySoild(chunk.fetch(x, y, z));
}

auto OMVoxelCompiler::queryBlockstate(const world::OMChunk<16> &chunk,
                                      std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                      int x, int y, int z) -> uint32_t
{
    if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
    {
        return externalAccessor(glm::ivec3(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz);
    }

    return chunk.fetch(x, y, z);
}

auto OMVoxelCompiler::computeAO(const world::OMChunk<16> &chunk,
                                std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor, int x,
                                int y, int z, OMVoxelFacing facing, int bsid, int pid)
    -> std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>
{
    uint8_t ao1 = 0, ao2 = 0, ao3 = 0, ao4 = 0;
    auto currentAabb = handler->queryPartAABB(bsid, pid);
    // INFO: ambientocculusion property is needed!
    auto countNeighborsPoint = [&](int dx, int dy, int dz, glm::ivec3 pos) -> int {
        auto tgbs = queryBlockstate(chunk, externalAccessor, x + dx, y + dy, z + dz);
        if (tgbs == 0)
            return 0;

        auto currentPos =
            glm::mix(currentAabb.offset, currentAabb.offset + currentAabb.size, glm::vec3(pos.x, pos.y, pos.z)) -
            (glm::ivec3(dx, dy, dz) * 16);

        auto occ = handler->queryOcculusionShape(tgbs);
        if (occ.contains(currentPos))
        {
            return 1;
        }

        return 0;
    };
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
        ao1 = finalAO(countNeighborsPoint(-1, -1, -1, {0, 0, 0}), countNeighborsPoint(-1, -1, 0, {0, 0, 0}),
                      countNeighborsPoint(0, -1, -1, {0, 0, 0}));
        ao2 = finalAO(countNeighborsPoint(-1, -1, 1, {0, 0, 1}), countNeighborsPoint(-1, -1, 0, {0, 0, 1}),
                      countNeighborsPoint(0, -1, 1, {0, 0, 1}));
        ao3 = finalAO(countNeighborsPoint(1, -1, -1, {1, 0, 0}), countNeighborsPoint(1, -1, 0, {1, 0, 0}),
                      countNeighborsPoint(0, -1, -1, {1, 0, 0}));
        ao4 = finalAO(countNeighborsPoint(1, -1, 1, {1, 0, 1}), countNeighborsPoint(1, -1, 0, {1, 0, 1}),
                      countNeighborsPoint(0, -1, 1, {1, 0, 1}));
        break;
    }
    case OMVoxelFacing::PosY: {
        ao1 = finalAO(countNeighborsPoint(-1, 1, -1, {0, 1, 0}), countNeighborsPoint(-1, 1, 0, {0, 1, 0}),
                      countNeighborsPoint(0, 1, -1, {0, 1, 0}));
        ao2 = finalAO(countNeighborsPoint(-1, 1, 1, {0, 1, 1}), countNeighborsPoint(-1, 1, 0, {0, 1, 1}),
                      countNeighborsPoint(0, 1, 1, {0, 1, 1}));
        ao3 = finalAO(countNeighborsPoint(1, 1, -1, {1, 1, 0}), countNeighborsPoint(1, 1, 0, {1, 1, 0}),
                      countNeighborsPoint(0, 1, -1, {1, 1, 0}));
        ao4 = finalAO(countNeighborsPoint(1, 1, 1, {1, 1, 1}), countNeighborsPoint(1, 1, 0, {1, 1, 1}),
                      countNeighborsPoint(0, 1, 1, {1, 1, 1}));
        break;
    }
    case OMVoxelFacing::NegX: {
        ao1 = finalAO(countNeighborsPoint(-1, 1, -1, {0, 1, 0}), countNeighborsPoint(-1, 1, 0, {0, 1, 0}),
                      countNeighborsPoint(-1, 0, -1, {0, 1, 0}));
        ao2 = finalAO(countNeighborsPoint(-1, -1, -1, {0, 0, 0}), countNeighborsPoint(-1, -1, 0, {0, 0, 0}),
                      countNeighborsPoint(-1, 0, -1, {0, 0, 0}));
        ao3 = finalAO(countNeighborsPoint(-1, 1, 1, {0, 1, 1}), countNeighborsPoint(-1, 1, 0, {0, 1, 1}),
                      countNeighborsPoint(-1, 0, 1, {0, 1, 1}));
        ao4 = finalAO(countNeighborsPoint(-1, -1, 1, {0, 0, 1}), countNeighborsPoint(-1, -1, 0, {0, 0, 1}),
                      countNeighborsPoint(-1, 0, 1, {0, 0, 1}));
        break;
    }
    case OMVoxelFacing::PosX: {
        ao1 = finalAO(countNeighborsPoint(1, 1, 1, {1, 1, 1}), countNeighborsPoint(1, 1, 0, {1, 1, 1}),
                      countNeighborsPoint(1, 0, 1, {1, 1, 1}));
        ao2 = finalAO(countNeighborsPoint(1, -1, 1, {1, 0, 1}), countNeighborsPoint(1, -1, 0, {1, 0, 1}),
                      countNeighborsPoint(1, 0, 1, {1, 0, 1}));
        ao3 = finalAO(countNeighborsPoint(1, 1, -1, {1, 1, 0}), countNeighborsPoint(1, 1, 0, {1, 1, 0}),
                      countNeighborsPoint(1, 0, -1, {1, 1, 0}));
        ao4 = finalAO(countNeighborsPoint(1, -1, -1, {1, 0, 0}), countNeighborsPoint(1, -1, 0, {1, 0, 0}),
                      countNeighborsPoint(1, 0, -1, {1, 0, 0}));
        break;
    }
    case OMVoxelFacing::NegZ: {
        ao1 = finalAO(countNeighborsPoint(1, 1, -1, {1, 1, 0}), countNeighborsPoint(1, 0, -1, {1, 1, 0}),
                      countNeighborsPoint(0, 1, -1, {1, 1, 0}));
        ao2 = finalAO(countNeighborsPoint(1, -1, -1, {1, 0, 0}), countNeighborsPoint(1, 0, -1, {1, 0, 0}),
                      countNeighborsPoint(0, -1, -1, {1, 0, 0}));
        ao3 = finalAO(countNeighborsPoint(-1, 1, -1, {0, 1, 0}), countNeighborsPoint(-1, 0, -1, {0, 1, 0}),
                      countNeighborsPoint(0, 1, -1, {0, 1, 0}));
        ao4 = finalAO(countNeighborsPoint(-1, -1, -1, {0, 0, 0}), countNeighborsPoint(-1, 0, -1, {0, 0, 0}),
                      countNeighborsPoint(0, -1, -1, {0, 0, 0}));
        break;
    }
    case OMVoxelFacing::PosZ: {
        ao1 = finalAO(countNeighborsPoint(-1, 1, 1, {0, 1, 1}), countNeighborsPoint(-1, 0, 1, {0, 1, 1}),
                      countNeighborsPoint(0, 1, 1, {0, 1, 1}));
        ao2 = finalAO(countNeighborsPoint(-1, -1, 1, {0, 0, 1}), countNeighborsPoint(-1, 0, 1, {0, 0, 1}),
                      countNeighborsPoint(0, -1, 1, {0, 0, 1}));
        ao3 = finalAO(countNeighborsPoint(1, 1, 1, {1, 1, 1}), countNeighborsPoint(1, 0, 1, {1, 1, 1}),
                      countNeighborsPoint(0, 1, 1, {1, 1, 1}));
        ao4 = finalAO(countNeighborsPoint(1, -1, 1, {1, 0, 1}), countNeighborsPoint(1, 0, 1, {1, 0, 1}),
                      countNeighborsPoint(0, -1, 1, {1, 0, 1}));
        break;
    }
    default:
        break;
    }
    return {ao1, ao2, ao3, ao4};
}

auto OMVoxelCompiler::checkExistSoild(const world::OMChunk<16> &chunk,
                                      std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                      glm::ivec3 v, OMVoxelFacing f) -> bool
{
    switch (f)
    {
    case None:
        return false;
    case NegX:
        return existSoild(chunk, externalAccessor, v.x - 1, v.y, v.z);
    case NegY:
        return existSoild(chunk, externalAccessor, v.x, v.y - 1, v.z);
    case NegZ:
        return existSoild(chunk, externalAccessor, v.x, v.y, v.z - 1);
    case PosX:
        return existSoild(chunk, externalAccessor, v.x + 1, v.y, v.z);
    case PosY:
        return existSoild(chunk, externalAccessor, v.x, v.y + 1, v.z);
    case PosZ:
        return existSoild(chunk, externalAccessor, v.x, v.y, v.z + 1);
    }
}

auto OMVoxelCompiler::compile(const world::OMChunk<16> &chunk,
                              std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                              int chunkid, std::function<void(OMVoxel)> commiter) -> void
{
    for (const auto &v : chunk)
    {
        if (handler->queryNumParts(v.second) == 0)
        {
            continue;
        }

        for (int i = 0; i < handler->queryNumParts(v.second); ++i)
        {
            for (auto f : {NegX, NegY, NegZ, PosX, PosY, PosZ})
            {
                auto ff = handler->queryPartFaceCull(v.second, i, f);
                if (handler->queryPartFaceEnabled(v.second, i, f) &&
                    !checkExistSoild(chunk, externalAccessor, v.first, handler->queryPartFaceCull(v.second, i, f)))
                {
                    auto [ao1, ao2, ao3, ao4] =
                        computeAO(chunk, externalAccessor, v.first.x, v.first.y, v.first.z, f, v.second, i);
                    auto aabb = handler->queryPartAABB(v.second, i);
                    auto uv = handler->queryPartFaceUV(v.second, i, f);
                    auto raxis = handler->queryPartRotationCenter(v.second, i);
                    auto vox =
                        packVoxel(v.first.x, v.first.y, v.first.z, f, std::abs(aabb.offset.x), std::abs(aabb.offset.y),
                                  std::abs(aabb.offset.z), aabb.offset.x < 0, aabb.offset.y < 0, aabb.offset.z < 0,
                                  handler->queryPartFaceTex(v.second, i, f), chunkid,
                                  handler->queryPartFaceRotation(v.second, i, f), 15, 15, 15, 15, 0, 0, 0, 0,
                                  aabb.size.x, aabb.size.y, aabb.size.z, ao1, ao2, ao3, ao4, uv.x, uv.y, uv.z, uv.w,
                                  handler->queryPartRotationAxis(v.second, i), std::abs(raxis.x), std::abs(raxis.y),
                                  std::abs(raxis.z), raxis.x < 0, raxis.y < 0, raxis.z < 0,
                                  handler->queryPartRotationAngle(v.second, i));
                    commiter(OMVoxel{vox[0], vox[1], vox[2], vox[3], vox[4]});
                }
            }
        }
    }
}
} // namespace openminecraft::renderer::common::wrap