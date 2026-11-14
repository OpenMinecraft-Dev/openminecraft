#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_compiler.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <tuple>
#include <unordered_map>

namespace openminecraft::renderer::common::wrap::voxel
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
// S -> Voxel Fluid Source flag (1 bit)
// M -> Voxel colormap id (24 bits)
// u -> unused
// xxxx yyyy zzzz efff uuuu uuuu uuuu cccS
// uutt tttt tttt tttt cccc cccc cccc cccc
// llll llll llll llll LLLL LLLL LLLL LLLL
// hhhh hhhh hhhh hhhh hhhh hhhh hhhh hhhh
// uuuu uuuu MMMM MMMM MMMM MMMM MMMM MMMM
static constexpr auto packVoxelFluid(uint8_t x, uint8_t y, uint8_t z, OMVoxelFacing facing, uint16_t tex, int32_t chkid,
                                     uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t bl1, uint8_t bl2,
                                     uint8_t bl3, uint8_t bl4, uint8_t h1, uint8_t h2, uint8_t h3, uint8_t h4,
                                     bool source, uint8_t colormapr, uint8_t colormapg, uint8_t colormapb)
    -> std::array<int, 5>
{
    return {
        x << 28 | y << 24 | z << 20 | 1 << 19 | (facing & 7) << 16 | (((chkid >> 16) & 7) << 1) | source,
        ((tex & 0x3fff) << 16) | (chkid & 0xffff),
        ((l1 & 0xf) << 28) | ((l2 & 0xf) << 24) | ((l3 & 0xf) << 20) | ((l4 & 0xf) << 16) | ((bl1 & 0xf) << 12) |
            ((bl2 & 0xf) << 8) | ((bl3 & 0xf) << 4) | (bl4 & 0xf),
        h1 << 24 | h2 << 16 | h3 << 8 | h4,
        colormapr << 16 | colormapg << 8 | colormapb,
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
// M -> Voxel colormap (24 bits)
// u -> unused
// INFO: packed vertex structure in u32
// xxxx yyyy zzzz efff XXXX YYYY ZZZZ cccU
// rrtt tttt tttt tttt cccc cccc cccc cccc
// llll llll llll llll LLLL LLLL LLLL LLLL
// ssss ssss ssss sssU UUUU UUUU aaaa aaaa
// UUUU UUUU UUAA CCCC CCCC CCCC CCCu Snnn
// uuuu uuuu MMMM MMMM MMMM MMMM MMMM MMMM
static constexpr auto packVoxel(uint8_t x, uint8_t y, uint8_t z, OMVoxelFacing facing, uint8_t dx, uint8_t dy,
                                uint8_t dz, uint16_t tex, int32_t chkid, uint8_t rotation, uint8_t l1, uint8_t l2,
                                uint8_t l3, uint8_t l4, uint8_t bl1, uint8_t bl2, uint8_t bl3, uint8_t bl4,
                                uint8_t scaleX, uint8_t scaleY, uint8_t scaleZ, uint8_t ao1, uint8_t ao2, uint8_t ao3,
                                uint8_t ao4, uint8_t u0, uint8_t v0, uint8_t u1, uint8_t v1, uint8_t rotationAxis,
                                uint8_t rotationCx, uint8_t rotationCy, uint8_t rotationCz, bool rCxNeg, bool rCyNeg,
                                bool rCzNeg, uint8_t rAngle, bool shade, uint8_t colormapr, uint8_t colormapg,
                                uint8_t colormapb) -> std::array<int, 6>
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
            (shade << 3) | (rAngle & 7),
        colormapr << 16 | colormapg << 8 | colormapb,
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
// X/Y/Z -> Voxel Div Coordinate (3 floats)
// U -> Voxel UV Offset (4 floats)
// A -> Voxel Rotation Axis (2 bits)
// n -> Voxel Rotation Angle (1 float)
// C -> Voxel Rotation Center (3 floats)
// s -> Voxel Uses Secondary Texture (1 bit)
// c -> Voxel Size (3 floats)
// M -> Voxel colormap (24 bits)
// u -> unused
// INFO: packed vertex structure in u32
// xxxx yyyy zzzz efff cccs AASu aaaa aaaa
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
// uuuu uuuu MMMM MMMM MMMM MMMM MMMM MMMM
static constexpr auto packVoxelComplex(uint8_t x, uint8_t y, uint8_t z, OMVoxelFacing facing, bool texSec, uint16_t tex,
                                       int32_t chkid, uint8_t rotation, uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4,
                                       uint8_t bl1, uint8_t bl2, uint8_t bl3, uint8_t bl4, uint8_t ao1, uint8_t ao2,
                                       uint8_t ao3, uint8_t ao4, bool shade, glm::vec3 modelOffset, glm::vec2 uv0,
                                       glm::vec2 uv1, uint8_t rotationAxis, float rotationAngle,
                                       glm::vec3 rotationCenter, glm::vec3 modelSize, glm::vec3 angleExt,
                                       uint8_t colormapr, uint8_t colormapg, uint8_t colormapb)
    -> std::tuple<int32_t, int32_t, int32_t, glm::vec3, glm::vec2, glm::vec2, float, glm::vec3, glm::vec3, float, float,
                  int32_t>
{
    return std::make_tuple(x << 28 | y << 24 | z << 20 | 1 << 19 | (facing & 7) << 16 | (((chkid >> 16) & 7) << 13) |
                               (texSec << 12) | ((rotationAxis & 3) << 10) | (shade << 9) | (ao1 << 6) | (ao2 << 4) |
                               (ao3 << 2) | ao4,
                           (rotation << 30) | ((tex & 0x3fff) << 16) | (chkid & 0xffff),
                           ((l1 & 0xf) << 28) | ((l2 & 0xf) << 24) | ((l3 & 0xf) << 20) | ((l4 & 0xf) << 16) |
                               ((bl1 & 0xf) << 12) | ((bl2 & 0xf) << 8) | ((bl3 & 0xf) << 4) | (bl4 & 0xf),
                           modelOffset, uv0, uv1, rotationAxis == 3 ? angleExt.x : rotationAngle, rotationCenter,
                           modelSize, angleExt.y, angleExt.z, colormapr << 16 | colormapg << 8 | colormapb);
}

auto OMVoxelCompiler::existSoild(const world::OMChunk<16> &chunk,
                                 std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor, int x,
                                 int y, int z) -> bool
{
    if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
        return handler->querySoild(
            modelFetcher(externalAccessor(glm::ivec3(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz), chunk.chunkx,
                         chunk.chunky, chunk.chunkz, x, y, z));

    return handler->querySoild(modelFetcher(chunk.fetch(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z));
}

auto OMVoxelCompiler::isFullBlock(int modelId) -> bool
{
    return handler->queryFullBlock(modelId);
}

auto OMVoxelCompiler::fluidIsFullCell(
    const world::OMChunk<16> &chunk,
    std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor, glm::ivec3 v, int modelId) -> bool
{
    if (checkExistFluidSame(chunk, externalAccessor, v, PosY, modelId))
    {
        return true;
    }

    auto above = v;
    above.y += 1;
    const uint32_t aboveState = queryBlockState(chunk, externalAccessor, above.x, above.y, above.z);
    const int aboveModel = modelFetcher(aboveState, chunk.chunkx, chunk.chunky, chunk.chunkz, above.x, above.y, above.z);

    if (!isFullBlock(aboveModel))
    {
        return false;
    }

    // Ice/glass are full cube but translucent. Their lower water is an
    // interior cell: it must reach the block above, otherwise the lowered
    // top face becomes visible through the ice.
    if (handler->queryTranslucent(aboveModel))
    {
        return true;
    }

    static constexpr glm::ivec3 dirs[4] = {{-1, 0, 0}, {1, 0, 0}, {0, 0, -1}, {0, 0, 1}};

    for (const auto &d : dirs)
    {
        const glm::ivec3 p = v + d;
        const uint32_t state = queryBlockState(chunk, externalAccessor, p.x, p.y, p.z);
        const int model = modelFetcher(state, chunk.chunkx, chunk.chunky, chunk.chunkz, p.x, p.y, p.z);

        if (!handler->queryFluid(model) || !handler->queryFluidSame(model, modelId))
        {
            return false;
        }

        auto nAbove = p;
        nAbove.y += 1;
        const uint32_t naState = queryBlockState(chunk, externalAccessor, nAbove.x, nAbove.y, nAbove.z);
        const int naModel =
            modelFetcher(naState, chunk.chunkx, chunk.chunky, chunk.chunkz, nAbove.x, nAbove.y, nAbove.z);

        const bool covered = handler->queryFluidFalling(model) ||
                             checkExistFluidSame(chunk, externalAccessor, p, PosY, model) || isFullBlock(naModel);

        if (!covered)
        {
            return false;
        }
    }

    return true;
}

auto OMVoxelCompiler::existFullBlock(const world::OMChunk<16> &chunk,
                                     std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                     int x, int y, int z) -> bool
{
    const uint32_t state = queryBlockState(chunk, externalAccessor, x, y, z);
    const int model = modelFetcher(state, chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z);
    return isFullBlock(model);
}

auto OMVoxelCompiler::existFluidSame(const world::OMChunk<16> &chunk,
                                     std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                     int x, int y, int z, int modelId) -> bool
{
    bool exist;
    if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
    {
        exist = handler->queryFluid(
            modelFetcher(externalAccessor(glm::ivec3(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz), chunk.chunkx,
                         chunk.chunky, chunk.chunkz, x, y, z));
    }
    else
    {
        exist =
            handler->queryFluid(modelFetcher(chunk.fetch(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z));
    }
    return exist && handler->queryFluidSame(modelFetcher(queryBlockState(chunk, externalAccessor, x, y, z),
                                                         chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z),
                                            modelId);
}

auto OMVoxelCompiler::queryBlockState(const world::OMChunk<16> &chunk,
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
                                int y, int z, OMVoxelFacing facing, int modelId, int pid)
    -> std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>
{
    uint8_t ao1 = 0, ao2 = 0, ao3 = 0, ao4 = 0;
    auto currentAabb = handler->queryPartAABB(modelId, pid);
    auto countNeighborsPoint = [&](int dx, int dy, int dz, glm::ivec3 pos) -> int {
        auto tgbs = queryBlockState(chunk, externalAccessor, x + dx, y + dy, z + dz);
        if (tgbs == 0)
        {
            return 0;
        }

        auto currentPos =
            glm::mix(currentAabb.offset, currentAabb.offset + currentAabb.size, glm::vec3(pos.x, pos.y, pos.z)) -
            (glm::vec3(dx, dy, dz) * 16.0f);

        auto occ = handler->queryOcclusionShape(modelFetcher(tgbs, chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z));
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

auto OMVoxelCompiler::checkExistFluidSame(
    const world::OMChunk<16> &chunk, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
    glm::ivec3 v, OMVoxelFacing f, int state) -> bool
{
    switch (f)
    {
    default:
    case None:
        return false;
    case NegX:
        return existFluidSame(chunk, externalAccessor, v.x - 1, v.y, v.z, state);
    case NegY:
        return existFluidSame(chunk, externalAccessor, v.x, v.y - 1, v.z, state);
    case NegZ:
        return existFluidSame(chunk, externalAccessor, v.x, v.y, v.z - 1, state);
    case PosX:
        return existFluidSame(chunk, externalAccessor, v.x + 1, v.y, v.z, state);
    case PosY:
        return existFluidSame(chunk, externalAccessor, v.x, v.y + 1, v.z, state);
    case PosZ:
        return existFluidSame(chunk, externalAccessor, v.x, v.y, v.z + 1, state);
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
        target = queryBlockState(chunk, externalAccessor, v.x - 1, v.y, v.z);
        break;
    case NegY:
        target = queryBlockState(chunk, externalAccessor, v.x, v.y - 1, v.z);
        break;
    case NegZ:
        target = queryBlockState(chunk, externalAccessor, v.x, v.y, v.z - 1);
        break;
    case PosX:
        target = queryBlockState(chunk, externalAccessor, v.x + 1, v.y, v.z);
        break;
    case PosY:
        target = queryBlockState(chunk, externalAccessor, v.x, v.y + 1, v.z);
        break;
    case PosZ:
        target = queryBlockState(chunk, externalAccessor, v.x, v.y, v.z + 1);
        break;
    }
    return handler->querySkipsRendering(id, target, f);
}

auto OMVoxelCompiler::checkAvgFluid(const world::OMChunk<16> &chunk,
                                    std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                    glm::ivec3 v, int idx, int current) -> float
{
    uint32_t selfModel = modelFetcher(queryBlockState(chunk, externalAccessor, v.x, v.y, v.z), chunk.chunkx,
                                      chunk.chunky, chunk.chunkz, v.x, v.y, v.z);
    if (!handler->queryFluid(selfModel))
    {
        return 0.0f;
    }

    // If the block above is the same fluid, this cell is not a surface; use a full cell (9);
    // otherwise use vanilla's 8 - level (source water = 8, i.e. 8/9 of a block).
    const bool selfHasFluidAbove = checkExistFluidSame(chunk, externalAccessor, v, PosY, selfModel);

    // A full block above only fills the cell when the surrounding water is
    // also a covered column. A spring/edge with flowing neighbors stays lower.
    const bool selfIsFullCell = fluidIsFullCell(chunk, externalAccessor, v, selfModel);

    float tot = (selfHasFluidAbove || selfIsFullCell)
                    ? 9.0f
                    : (handler->queryFluidFalling(selfModel) ? 9.0f : (8.0f - current));
    int lev = 1;

    std::array<glm::ivec3, 3> nPos;
    switch (idx)
    {
    case 0:
        nPos = {{{v.x - 1, v.y, v.z - 1}, {v.x - 1, v.y, v.z}, {v.x, v.y, v.z - 1}}};
        break;
    case 2:
        nPos = {{{v.x + 1, v.y, v.z - 1}, {v.x + 1, v.y, v.z}, {v.x, v.y, v.z - 1}}};
        break;
    case 1:
        nPos = {{{v.x - 1, v.y, v.z + 1}, {v.x - 1, v.y, v.z}, {v.x, v.y, v.z + 1}}};
        break;
    case 3:
        nPos = {{{v.x + 1, v.y, v.z + 1}, {v.x + 1, v.y, v.z}, {v.x, v.y, v.z + 1}}};
        break;
    default:
        return 227.0f * (tot / 8.0f);
    }

    for (const auto &p : nPos)
    {
        uint32_t ns = modelFetcher(queryBlockState(chunk, externalAccessor, p.x, p.y, p.z), chunk.chunkx, chunk.chunky,
                                   chunk.chunkz, p.x, p.y, p.z);
        if (!handler->queryFluid(ns))
            continue;

        if (!handler->queryFluidSame(selfModel, ns))
        {
            continue;
        }

        if (handler->queryFluidFalling(ns) || fluidIsFullCell(chunk, externalAccessor, p, ns))
        {
            tot += 9.0f;
        }
        else
        {
            tot += 8.0f - (handler->queryFluidLevel(ns) & 0b111);
        }
        lev++;
    }

    return 255.0f * (tot / lev) / 9.0f;
}

auto OMVoxelCompiler::computeVisibility(const world::OMChunk<16> &chunk,
                                        std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor)
    -> uint64_t
{
    std::array<uint8_t, 4096> blocking = {};

    // Solidness is constant per blockstate; cache it to avoid per-block model resolution
    std::unordered_map<uint32_t, bool> solidCache;
    solidCache.reserve(256);

    for (int y = 0; y < 16; ++y)
    {
        for (int z = 0; z < 16; ++z)
        {
            for (int x = 0; x < 16; ++x)
            {
                const uint32_t state = chunk.fetch(x, y, z);
                auto it = solidCache.find(state);

                if (it == solidCache.end())
                {
                    const auto bsid = modelFetcher(state, chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z);
                    it = solidCache.emplace(state, handler->querySoild(bsid)).first;
                }

                if (it->second)
                {
                    blocking[y * 256 + z * 16 + x] = 1;
                }
            }
        }
    }

    return computeSectionVisibility(blocking);
}

auto OMVoxelCompiler::compile(const world::OMChunk<16> &chunk,
                              std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                              int chunkid, std::function<void(OMVoxel)> committer,
                              std::function<void(OMVoxelComplex)> committerComplex,
                              std::function<void(OMVoxelFluid)> committerFluid,
                              std::function<void(OMVoxel)> committerTranslucent,
                              std::function<void(OMVoxelComplex)> committerTranslucentComplex,
                              std::function<void(OMVoxelFluid)> committerTranslucentFluid) -> void
{
    for (const auto &v : chunk)
    {
        // INFO: bsid is the model id (mapped from the blockstate id); the blockstate id is v.second
        auto bsid = modelFetcher(v.second, chunk.chunkx, chunk.chunky, chunk.chunkz, v.first.x, v.first.y, v.first.z);
        auto trans = handler->queryTranslucent(bsid);

        if (handler->queryFluid(bsid))
        {
            const auto isFalling = handler->queryFluidFalling(bsid);
            const auto fluidLevel = (handler->queryFluidLevel(bsid) & 0b111);

            // Same fluid above => this cell is not a surface and should fill the whole cell;
            // only source water on the top surface layer is 8/9 of a block tall (vanilla behavior).
            const bool fluidAbove = checkExistFluidSame(chunk, externalAccessor, v.first, PosY, bsid);
            const bool fullBlock = isFalling || fluidAbove || fluidIsFullCell(chunk, externalAccessor, v.first, bsid);

            const std::array<float, 4> cornerHeights = {
                fullBlock ? 255.0f : checkAvgFluid(chunk, externalAccessor, v.first, 0, fluidLevel),
                fullBlock ? 255.0f : checkAvgFluid(chunk, externalAccessor, v.first, 1, fluidLevel),
                fullBlock ? 255.0f : checkAvgFluid(chunk, externalAccessor, v.first, 2, fluidLevel),
                fullBlock ? 255.0f : checkAvgFluid(chunk, externalAccessor, v.first, 3, fluidLevel),
            };

            // Fluid height comparison uses corner heights (0..255) on both sides of the shared edge:
            // corner heights average the same 4 neighboring cells, so both sides agree,
            // leaving no gaps from 1/255 quantization and ensuring each face is drawn once.
            auto cornerHeightOf = [&](const glm::ivec3 &p, int mdl, int idx) -> float {
                if (handler->queryFluidFalling(mdl) || checkExistFluidSame(chunk, externalAccessor, p, PosY, mdl))
                {
                    return 255.0f;
                }

                const auto level = static_cast<uint8_t>(handler->queryFluidLevel(mdl) & 0b111);
                return checkAvgFluid(chunk, externalAccessor, p, idx, level);
            };

            for (auto f : {NegX, NegY, NegZ, PosX, PosY, PosZ})
            {
                bool culled = false;

                if (f == NegX || f == PosX || f == NegZ || f == PosZ)
                {
                    // Side face: cannot be culled when the neighbor is the same fluid with a lower surface,
                    // otherwise waterfall/step flows show gaps on their sides
                    glm::ivec3 np = v.first;

                    switch (f)
                    {
                    case NegX:
                        np.x -= 1;
                        break;
                    case PosX:
                        np.x += 1;
                        break;
                    case NegZ:
                        np.z -= 1;
                        break;
                    case PosZ:
                        np.z += 1;
                        break;
                    default:
                        break;
                    }

                    const uint32_t nst = queryBlockState(chunk, externalAccessor, np.x, np.y, np.z);
                    const int nmdl = modelFetcher(nst, chunk.chunkx, chunk.chunky, chunk.chunkz, np.x, np.y, np.z);

                    if (isFullBlock(nmdl))
                    {
                        // The fluid side contacts a full cube. Its face is
                        // hidden inside the block and must not be rendered,
                        // matching vanilla fluid face culling. This also covers
                        // translucent full blocks such as ice.
                        culled = true;
                    }
                    else if (handler->queryFluid(nmdl) && handler->queryFluidSame(nmdl, bsid))
                    {
                        int selfA = 0, selfB = 0, nA = 0, nB = 0;

                        switch (f)
                        {
                        case PosX:
                            selfA = 2, selfB = 3, nA = 0, nB = 1;
                            break;
                        case NegX:
                            selfA = 0, selfB = 1, nA = 2, nB = 3;
                            break;
                        case PosZ:
                            selfA = 1, selfB = 3, nA = 0, nB = 2;
                            break;
                        default: // NegZ
                            selfA = 0, selfB = 2, nA = 1, nB = 3;
                            break;
                        }

                        const float selfH = std::max(cornerHeights[selfA], cornerHeights[selfB]);
                        const float neighbourH = std::max(cornerHeightOf(np, nmdl, nA), cornerHeightOf(np, nmdl, nB));

                        culled = neighbourH >= selfH - 1e-3f;
                    }
                }
                else
                {
                    culled = checkExistFluidSame(chunk, externalAccessor, v.first, f, bsid);

                    if (!culled)
                    {
                        glm::ivec3 np = v.first;
                        if (f == PosY)
                            np.y += 1;
                        else if (f == NegY)
                            np.y -= 1;

                        const uint32_t nst = queryBlockState(chunk, externalAccessor, np.x, np.y, np.z);
                        const int nmdl = modelFetcher(nst, chunk.chunkx, chunk.chunky, chunk.chunkz, np.x, np.y, np.z);

                        if (f == NegY)
                        {
                            // The bottom always touches the block below.
                            culled = isFullBlock(nmdl);
                        }
                        else if (f == PosY && isFullBlock(nmdl))
                        {
                            // The top is covered by a full block (including
                            // translucent ice/glass); it can never be visible
                            // from outside, even if the fluid surface is below 1.
                            culled = true;
                        }
                    }
                }

                if (!culled)
                {
                    auto useSec = fluidLevel != 0 || isFalling;

                    const glm::u8vec3 fluidTint =
                        paletteProvider ? paletteProvider->fluidColor(chunk, v.first.x, v.first.y, v.first.z, v.second)
                                        : glm::u8vec3{255, 255, 255};

                    auto vox =
                        packVoxelFluid(v.first.x, v.first.y, v.first.z, f,
                                       useSec ? handler->queryFluidTexSec(bsid) : handler->queryFluidTex(bsid), chunkid,
                                       15, 15, 15, 15, 0, 0, 0, 0, cornerHeights[0], cornerHeights[1], cornerHeights[2],
                                       cornerHeights[3], useSec, fluidTint.r, fluidTint.g, fluidTint.b);
                    (trans || handler->queryWaterlogged(bsid) ? committerTranslucentFluid : committerFluid)(
                        OMVoxelFluid{vox[0], vox[1], vox[2], vox[3], vox[4]});
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

                    const int tintIndex = handler->queryPartFaceTint(bsid, i, f);
                    const glm::u8vec3 tint =
                        (tintIndex >= 0 && paletteProvider)
                            ? paletteProvider->color(chunk, v.first.x, v.first.y, v.first.z, v.second, tintIndex)
                            : glm::u8vec3{255, 255, 255};

                    if (handler->queryPartComplex(bsid, i))
                    {
                        float rotationAngle = handler->queryPartRotationAngleF(bsid, i);
                        bool texSec = handler->queryPartFaceSecondaryTexture(bsid, i, f);
                        auto vox = packVoxelComplex(
                            v.first.x, v.first.y, v.first.z, f, texSec, handler->queryPartFaceTex(bsid, i, f), chunkid,
                            handler->queryPartFaceRotation(bsid, i, f), 15, 15, 15, 15, 0, 0, 0, 0, ao1, ao2, ao3, ao4,
                            handler->queryPartShade(bsid, i), aabb.offset, {uv.x, uv.y}, {uv.z, uv.w},
                            handler->queryPartRotationAxis(bsid, i), rotationAngle, raxis, aabb.size,
                            handler->queryPartRotationAngleExt(bsid, i), tint.r, tint.g, tint.b);
                        (trans ? committerTranslucentComplex : committerComplex)(
                            OMVoxelComplex{std::get<0>(vox), std::get<1>(vox), std::get<2>(vox), std::get<3>(vox),
                                           std::get<4>(vox), std::get<5>(vox), std::get<6>(vox), std::get<7>(vox),
                                           std::get<8>(vox), std::get<9>(vox), std::get<10>(vox), std::get<11>(vox)});
                        continue;
                    }

                    auto vox =
                        packVoxel(v.first.x, v.first.y, v.first.z, f, std::abs(aabb.offset.x), std::abs(aabb.offset.y),
                                  std::abs(aabb.offset.z), handler->queryPartFaceTex(bsid, i, f), chunkid,
                                  handler->queryPartFaceRotation(bsid, i, f), 15, 15, 15, 15, 0, 0, 0, 0, aabb.size.x,
                                  aabb.size.y, aabb.size.z, ao1, ao2, ao3, ao4, uv.x, uv.y, uv.z, uv.w,
                                  handler->queryPartRotationAxis(bsid, i), std::abs(raxis.x), std::abs(raxis.y),
                                  std::abs(raxis.z), raxis.x < 0, raxis.y < 0, raxis.z < 0,
                                  handler->queryPartRotationAngle(bsid, i), handler->queryPartShade(bsid, i), tint.r,
                                  tint.g, tint.b);
                    (trans ? committerTranslucent : committer)(OMVoxel{vox[0], vox[1], vox[2], vox[3], vox[4], vox[5]});
                }
            }
        }
    }
}
} // namespace openminecraft::renderer::common::wrap::voxel
