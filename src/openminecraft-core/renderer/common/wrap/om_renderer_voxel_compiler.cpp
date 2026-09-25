#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <tuple>

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
// f -> Voxel Facing (3 bits)
// e -> Voxel Enable (1 bit)
// t -> Voxel Texture Index (14 bits)
// c -> Voxel Chunk ID (19 bits)
// l -> Voxel Sky Light (4 * 4 bits)
// L -> Voxel Block Light (4 * 4 bits)
// h -> Voxel Fluid Corner Heights (4 * 8 bits)
// u -> unused
// xxxx yyyy zzzz efff uuuu uuuu uuuu cccu
// uutt tttt tttt tttt cccc cccc cccc cccc
// llll llll llll llll LLLL LLLL LLLL LLLL
// hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh
static constexpr auto packVoxelFluid(uint8_t x, uint8_t y, uint8_t z, OMVoxelFacing facing, uint16_t tex, int32_t chkid,
                                     uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t bl1, uint8_t bl2,
                                     uint8_t bl3, uint8_t bl4, uint8_t h1, uint8_t h2, uint8_t h3, uint8_t h4)
    -> std::array<int, 4>
{
    return {
        x << 28 | y << 24 | z << 20 | 1 << 19 | (facing & 7) << 16 | (((chkid >> 16) & 7) << 1),
        ((tex & 0x3fff) << 16) | (chkid & 0xffff),
        ((l1 & 0xf) << 28) | ((l2 & 0xf) << 24) | ((l3 & 0xf) << 20) | ((l4 & 0xf) << 16) | ((bl1 & 0xf) << 12) |
            ((bl2 & 0xf) << 8) | ((bl3 & 0xf) << 4) | (bl4 & 0xf),
        h1 << 24 | h2 << 16 | h3 << 8 | h4,
    };
}

// INFO: letter -> meanings
// x -> Voxel X Coordinate (4 bits)
// y -> Voxel Y Coordinate (4 bits)
// z -> Voxel Z Coordinate (4 bits)
// f -> Voxel Facing (3 bits)
// X -> Voxel X Coordinate Div (4 bits)
// Y -> Voxel Y Coordinate Div (4 bits)
// Z -> Voxel Z Coordinate Div (4 bits)
// s -> Voxel Scale (3 * 5 bits)
// e -> Voxel Enable (1 bit)
// a -> Voxel Ambient Occclusion Levels (4 * 2 bits)
// t -> Voxel Texture Index (14 bits)
// c -> Voxel Chunk ID (19 bits)
// r -> Voxel Rotation (2 bits, 00 -> 0deg, 01 -> 90deg, 10 -> 180deg, 11 -> 270deg)
// l -> Voxel Sky Light (4 * 4 bits)
// L -> Voxel Block Light (4 * 4 bits)
// U -> Voxel UV Offset (4 * 5 bits)
// A -> Voxel Rotation Axis (2 bits)
// C -> Voxel Rotation Center (3 * 5 bits)
// n -> Voxel Angle (3 bits)
// S -> Voxel Shade (1 bit)
// M -> Voxel Colormap (1 bit)
// u -> unused
// INFO: packed vertex structure in u32
// xxxx yyyy zzzz efff XXXX YYYY ZZZZ cccU
// rrtt tttt tttt tttt cccc cccc cccc cccc
// llll llll llll llll LLLL LLLL LLLL LLLL
// sssss sssss sssss U UUUU UUUU aaaa aaaa
// UUUUU UUUUU AA CCC CCCC CCCC CCCC MSnnn
static constexpr auto packVoxel(uint8_t x, uint8_t y, uint8_t z, OMVoxelFacing facing, uint8_t dx, uint8_t dy,
                                uint8_t dz, uint16_t tex, int32_t chkid, uint8_t rotation, uint8_t l1, uint8_t l2,
                                uint8_t l3, uint8_t l4, uint8_t bl1, uint8_t bl2, uint8_t bl3, uint8_t bl4,
                                uint8_t scaleX, uint8_t scaleY, uint8_t scaleZ, uint8_t ao1, uint8_t ao2, uint8_t ao3,
                                uint8_t ao4, uint8_t u0, uint8_t v0, uint8_t u1, uint8_t v1, uint8_t rotationAxis,
                                uint8_t rotationCx, uint8_t rotationCy, uint8_t rotationCz, bool rCxNeg, bool rCyNeg,
                                bool rCzNeg, uint8_t rAngle, bool shade, bool colormap) -> std::array<int, 5>
{
    return {
        x << 28 | y << 24 | z << 20 | 1 << 19 | (facing & 7) << 16 | dx << 12 | dy << 8 | dz << 4 |
            (((chkid >> 16) & 7) << 1) | ((u0 >> 4) & 1),
        (rotation << 30) | ((tex & 0x3fff) << 16) | (chkid & 0xffff),
        ((l1 & 0xf) << 28) | ((l2 & 0xf) << 24) | ((l3 & 0xf) << 20) | ((l4 & 0xf) << 16) | ((bl1 & 0xf) << 12) |
            ((bl2 & 0xf) << 8) | ((bl3 & 0xf) << 4) | (bl4 & 0xf),
        ((scaleX & 0x1f) << 27) | ((scaleY & 0x1f) << 22) | ((scaleZ & 0x1f) << 17) | ao1 << 6 | ao2 << 4 | ao3 << 2 |
            ao4 | ((v0 & 0x1f) << 12) | ((u0 & 0xf) << 8),
        ((u1 & 0x1f) << 27) | ((v1 & 0x1f) << 22) | ((rotationAxis & 3) << 20) | (rCxNeg << 19) | (rCyNeg << 18) |
            (rCzNeg << 17) | ((rotationCx & 0xf) << 13) | ((rotationCy & 0xf) << 9) | ((rotationCz & 0xf) << 5) |
            (colormap << 4) | (shade << 3) | (rAngle & 7),
    };
}
// INFO: letter -> meanings
// x -> Voxel X Coordinate (4 bits)
// y -> Voxel Y Coordinate (4 bits)
// z -> Voxel Z Coordinate (4 bits)
// f -> Voxel Facing (3 bits)
// e -> Voxel Enable (1 bit)
// a -> Voxel Ambient Occclusion Levels (4 * 2 bits)
// t -> Voxel Texture Index (14 bits)
// c -> Voxel Chunk ID (19 bits)
// r -> Voxel Rotation (2 bits, 00 -> 0deg, 01 -> 90deg, 10 -> 180deg, 11 -> 270deg)
// l -> Voxel Sky Light (4 * 4 bits)
// L -> Voxel Block Light (4 * 4 bits)
// S -> Voxel Shade (1 bit)
// M -> Voxel Colormap (1 bit)
// X/Y/Z -> Voxel Div Coordinate (3 floats)
// U -> Voxel UV Offset (4 floats)
// A -> Voxel Rotation Axis (2 bits)
// n -> Voxel Rotation Angle (1 float)
// C -> Voxel Rotation Center (3 floats)
// s -> Voxel Uses Secondary Texture (1 bit)
// c -> Voxel Size (3 floats)
// u -> unused
// INFO: packed vertex structure in u32
// xxxx yyyy zzzz efff cccs AASM aaaa aaaa
// rrtt tttt tttt tttt cccc cccc cccc cccc
// llll llll llll llll LLLL LLLL LLLL LLLL
// XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX
// YYYY YYYY YYYY YYYY YYYY YYYY YYYY YYYY
// ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ
// UUUU UUUU UUUU UUUU UUUU UUUU UUUU UUUU
// UUUU UUUU UUUU UUUU UUUU UUUU UUUU UUUU
// UUUU UUUU UUUU UUUU UUUU UUUU UUUU UUUU
// UUUU UUUU UUUU UUUU UUUU UUUU UUUU UUUU
// nnnn nnnn nnnn nnnn nnnn nnnn nnnn nnnn
// CCCC CCCC CCCC CCCC CCCC CCCC CCCC CCCC
// CCCC CCCC CCCC CCCC CCCC CCCC CCCC CCCC
// CCCC CCCC CCCC CCCC CCCC CCCC CCCC CCCC
// cccc cccc cccc cccc cccc cccc cccc cccc
// cccc cccc cccc cccc cccc cccc cccc cccc
// cccc cccc cccc cccc cccc cccc cccc cccc
static constexpr auto packVoxelComplex(uint8_t x, uint8_t y, uint8_t z, OMVoxelFacing facing, bool texSec, uint16_t tex,
                                       int32_t chkid, uint8_t rotation, uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4,
                                       uint8_t bl1, uint8_t bl2, uint8_t bl3, uint8_t bl4, uint8_t ao1, uint8_t ao2,
                                       uint8_t ao3, uint8_t ao4, bool shade, bool colormap, glm::vec3 modelOffset,
                                       glm::vec2 uv0, glm::vec2 uv1, uint8_t rotationAxis, float rotationAngle,
                                       glm::vec3 rotationCenter, glm::vec3 modelSize, glm::vec3 angleExt)
    -> std::tuple<int32_t, int32_t, int32_t, glm::vec3, glm::vec2, glm::vec2, float, glm::vec3, glm::vec3, float, float>
{
    return std::make_tuple(x << 28 | y << 24 | z << 20 | 1 << 19 | (facing & 7) << 16 | (((chkid >> 16) & 7) << 13) |
                               (texSec << 12) | ((rotationAxis & 3) << 10) | (shade << 9) | (colormap << 8) |
                               (ao1 << 6) | (ao2 << 4) | (ao3 << 2) | ao4,
                           (rotation << 30) | ((tex & 0x3fff) << 16) | (chkid & 0xffff),
                           ((l1 & 0xf) << 28) | ((l2 & 0xf) << 24) | ((l3 & 0xf) << 20) | ((l4 & 0xf) << 16) |
                               ((bl1 & 0xf) << 12) | ((bl2 & 0xf) << 8) | ((bl3 & 0xf) << 4) | (bl4 & 0xf),
                           modelOffset, uv0, uv1, rotationAxis == 3 ? angleExt.x : rotationAngle, rotationCenter,
                           modelSize, angleExt.y, angleExt.z);
}

auto OMVoxelCompiler::existSoild(const world::OMChunk<16> &chunk,
                                 std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor, int x,
                                 int y, int z) -> bool
{
    if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
        return handler->querySoild(
            converter(externalAccessor(glm::ivec3(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz), chunk.chunkx,
                      chunk.chunky, chunk.chunkz, x, y, z));

    return handler->querySoild(converter(chunk.fetch(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z));
}

auto OMVoxelCompiler::existFluid(const world::OMChunk<16> &chunk,
                                 std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor, int x,
                                 int y, int z) -> bool
{
    if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
        return handler->queryFluid(
            converter(externalAccessor(glm::ivec3(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz), chunk.chunkx,
                      chunk.chunky, chunk.chunkz, x, y, z));

    return handler->queryFluid(converter(chunk.fetch(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z));
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
        {
            return 0;
        }

        auto currentPos =
            glm::mix(currentAabb.offset, currentAabb.offset + currentAabb.size, glm::vec3(pos.x, pos.y, pos.z)) -
            (glm::vec3(dx, dy, dz) * 16.0f);

        auto occ = handler->queryOcclusionShape(converter(tgbs, chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z));
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
    default:
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

auto OMVoxelCompiler::checkExistFluid(const world::OMChunk<16> &chunk,
                                      std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                      glm::ivec3 v, OMVoxelFacing f) -> bool
{
    switch (f)
    {
    default:
    case None:
        return false;
    case NegX:
        return existFluid(chunk, externalAccessor, v.x - 1, v.y, v.z);
    case NegY:
        return existFluid(chunk, externalAccessor, v.x, v.y - 1, v.z);
    case NegZ:
        return existFluid(chunk, externalAccessor, v.x, v.y, v.z - 1);
    case PosX:
        return existFluid(chunk, externalAccessor, v.x + 1, v.y, v.z);
    case PosY:
        return existFluid(chunk, externalAccessor, v.x, v.y + 1, v.z);
    case PosZ:
        return existFluid(chunk, externalAccessor, v.x, v.y, v.z + 1);
    }
}

auto OMVoxelCompiler::checkSkip(const world::OMChunk<16> &chunk,
                                std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                glm::ivec3 v, OMVoxelFacing f, uint32_t id) -> bool
{
    uint32_t target = 0;
    switch (f)
    {
    default:
    case None:
        return false;
    case NegX:
        target = queryBlockstate(chunk, externalAccessor, v.x - 1, v.y, v.z);
        break;
    case NegY:
        target = queryBlockstate(chunk, externalAccessor, v.x, v.y - 1, v.z);
        break;
    case NegZ:
        target = queryBlockstate(chunk, externalAccessor, v.x, v.y, v.z - 1);
        break;
    case PosX:
        target = queryBlockstate(chunk, externalAccessor, v.x + 1, v.y, v.z);
        break;
    case PosY:
        target = queryBlockstate(chunk, externalAccessor, v.x, v.y + 1, v.z);
        break;
    case PosZ:
        target = queryBlockstate(chunk, externalAccessor, v.x, v.y, v.z + 1);
        break;
    }
    return handler->querySkipsRendering(id, target, f);
}

auto OMVoxelCompiler::checkAvgFluid(const world::OMChunk<16> &chunk,
                                    std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                    glm::ivec3 v, int idx) -> float
{
    std::array<int, 3> neighbours = {};

    switch (idx)
    {
    case 0:
        neighbours[0] = queryBlockstate(chunk, externalAccessor, v.x - 1, v.y, v.z - 1);
        neighbours[1] = queryBlockstate(chunk, externalAccessor, v.x - 1, v.y, v.z);
        neighbours[2] = queryBlockstate(chunk, externalAccessor, v.x, v.y, v.z - 1);
        break;
    case 2:
        neighbours[0] = queryBlockstate(chunk, externalAccessor, v.x - 1, v.y, v.z + 1);
        neighbours[1] = queryBlockstate(chunk, externalAccessor, v.x - 1, v.y, v.z);
        neighbours[2] = queryBlockstate(chunk, externalAccessor, v.x, v.y, v.z + 1);
        break;
    case 1:
        neighbours[0] = queryBlockstate(chunk, externalAccessor, v.x + 1, v.y, v.z - 1);
        neighbours[1] = queryBlockstate(chunk, externalAccessor, v.x + 1, v.y, v.z);
        neighbours[2] = queryBlockstate(chunk, externalAccessor, v.x, v.y, v.z - 1);
        break;
    case 3:
        neighbours[0] = queryBlockstate(chunk, externalAccessor, v.x + 1, v.y, v.z + 1);
        neighbours[1] = queryBlockstate(chunk, externalAccessor, v.x + 1, v.y, v.z);
        neighbours[2] = queryBlockstate(chunk, externalAccessor, v.x, v.y, v.z + 1);
        break;
    default:
        break;
    }

    auto lev = 0;
    auto tot = 0.0f;

    for (int i = 0; i < 3; ++i)
    {
        if (handler->queryFluid(neighbours[i]))
        {
            lev++;
            tot += (handler->queryFluidLevel(neighbours[i]) & 0b111);
        }
    }

    return lev ? 227.0f * (tot / lev) / 7.0f : 0.0f;
}

auto OMVoxelCompiler::compile(const world::OMChunk<16> &chunk,
                              std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                              int chunkid, std::function<void(OMVoxel)> commiter,
                              std::function<void(OMVoxelComplex)> commiterComplex,
                              std::function<void(OMVoxelFluid)> committerFluid,
                              std::function<void(OMVoxel)> commiterTranslucent,
                              std::function<void(OMVoxelComplex)> commiterTranslucentComplex,
                              std::function<void(OMVoxelFluid)> commiterTranslucentFluid) -> void
{
    for (const auto &v : chunk)
    {
        auto bsid = converter(v.second, chunk.chunkx, chunk.chunky, chunk.chunkz, v.first.x, v.first.y, v.first.z);
        auto trans = handler->queryTranslucent(bsid);

        if (handler->queryFluid(bsid))
        {
            auto a = handler->queryFluidFalling(bsid);
            auto b = handler->queryFluidLevel(bsid) & 0b111;

            auto currentlev = 227.0f * b / 7.0f;
            auto h1 = a ? 255.0f : (currentlev + checkAvgFluid(chunk, externalAccessor, v.first, 0)) / 2;
            auto h2 = a ? 255.0f : (currentlev + checkAvgFluid(chunk, externalAccessor, v.first, 1)) / 2;
            auto h3 = a ? 255.0f : (currentlev + checkAvgFluid(chunk, externalAccessor, v.first, 2)) / 2;
            auto h4 = a ? 255.0f : (currentlev + checkAvgFluid(chunk, externalAccessor, v.first, 3)) / 2;

            for (auto f : {NegX, NegY, NegZ, PosX, PosY, PosZ})
            {
                if (!checkExistFluid(chunk, externalAccessor, v.first, f))
                {
                    auto vox = packVoxelFluid(v.first.x, v.first.y, v.first.z, f, handler->queryFluidTex(bsid), chunkid,
                                              15, 15, 15, 15, 0, 0, 0, 0, h1, h2, h3, h4);
                    commiterTranslucentFluid(OMVoxelFluid{vox[0], vox[1], vox[2], vox[3]});
                }
            }
        }

        if (handler->queryNumParts(bsid) == 0)
        {
            continue;
        }

        for (int i = 0; i < handler->queryNumParts(bsid); ++i)
        {
            for (auto f : {NegX, NegY, NegZ, PosX, PosY, PosZ})
            {
                if (handler->queryPartFaceEnabled(bsid, i, f) &&
                    !checkExistSoild(chunk, externalAccessor, v.first, handler->queryPartFaceCull(bsid, i, f)) &&
                    !checkSkip(chunk, externalAccessor, v.first, handler->queryPartFaceCull(bsid, i, f), v.second))
                {
                    auto [ao1, ao2, ao3, ao4] =
                        handler->queryPartAmbientOcclusion(bsid, i) && handler->queryPartShade(bsid, i)
                            ? computeAO(chunk, externalAccessor, v.first.x, v.first.y, v.first.z, f, bsid, i)
                            : std::make_tuple<uint8_t, uint8_t, uint8_t, uint8_t>(0, 0, 0, 0);
                    auto aabb = handler->queryPartAABB(bsid, i);
                    auto uv = handler->queryPartFaceUV(bsid, i, f);
                    auto raxis = handler->queryPartRotationCenter(bsid, i);

                    if (handler->queryPartComplex(bsid, i))
                    {
                        float rotationAngle = handler->queryPartRotationAngleF(bsid, i);
                        bool texSec = handler->queryPartFaceSecondaryTexture(bsid, i, f);
                        auto vox = packVoxelComplex(
                            v.first.x, v.first.y, v.first.z, f, texSec, handler->queryPartFaceTex(bsid, i, f), chunkid,
                            handler->queryPartFaceRotation(bsid, i, f), 15, 15, 15, 15, 0, 0, 0, 0, ao1, ao2, ao3, ao4,
                            handler->queryPartShade(bsid, i), false, aabb.offset, {uv.x, uv.y}, {uv.z, uv.w},
                            handler->queryPartRotationAxis(bsid, i), rotationAngle, raxis, aabb.size,
                            handler->queryPartRotationAngleExt(bsid, i));
                        (trans ? commiterTranslucentComplex : commiterComplex)(
                            OMVoxelComplex{std::get<0>(vox), std::get<1>(vox), std::get<2>(vox), std::get<3>(vox),
                                           std::get<4>(vox), std::get<5>(vox), std::get<6>(vox), std::get<7>(vox),
                                           std::get<8>(vox), std::get<9>(vox), std::get<10>(vox)});
                        continue;
                    }

                    auto vox =
                        packVoxel(v.first.x, v.first.y, v.first.z, f, std::abs(aabb.offset.x), std::abs(aabb.offset.y),
                                  std::abs(aabb.offset.z), handler->queryPartFaceTex(bsid, i, f), chunkid,
                                  handler->queryPartFaceRotation(bsid, i, f), 15, 15, 15, 15, 0, 0, 0, 0, aabb.size.x,
                                  aabb.size.y, aabb.size.z, ao1, ao2, ao3, ao4, uv.x, uv.y, uv.z, uv.w,
                                  handler->queryPartRotationAxis(bsid, i), std::abs(raxis.x), std::abs(raxis.y),
                                  std::abs(raxis.z), raxis.x < 0, raxis.y < 0, raxis.z < 0,
                                  handler->queryPartRotationAngle(bsid, i), handler->queryPartShade(bsid, i), false);
                    (trans ? commiterTranslucent : commiter)(OMVoxel{vox[0], vox[1], vox[2], vox[3], vox[4]});
                }
            }
        }
    }
}
} // namespace openminecraft::renderer::common::wrap
