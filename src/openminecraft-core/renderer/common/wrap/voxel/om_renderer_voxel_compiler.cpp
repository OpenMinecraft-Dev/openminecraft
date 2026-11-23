#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_compiler.hpp"
#include <algorithm>
#include <array>
#include <cmath>
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
        if (tgbs == airId.load())
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

auto OMVoxelCompiler::checkSkipRendering(
    const world::OMChunk<16> &chunk, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
    glm::ivec3 v, OMVoxelFacing f, uint32_t id) -> bool
{
    switch (f)
    {
    default:
    case None:
        return false;
    case NegX:
        v.x -= 1;
        break;
    case NegY:
        v.y -= 1;
        break;
    case NegZ:
        v.z -= 1;
        break;
    case PosX:
        v.x += 1;
        break;
    case PosY:
        v.y += 1;
        break;
    case PosZ:
        v.z += 1;
        break;
    }
    return handler->querySkipsRendering(id, queryBlockState(chunk, externalAccessor, v.x, v.y, v.z), f);
}

namespace
{
constexpr float faceEpsilon = 1e-4f;

struct FacePlane
{
    int axis;
    int u;
    int v;
    bool positive;
    float uMin;
    float uMax;
    float vMin;
    float vMax;
    float plane;
};

struct FaceRect
{
    float uMin;
    float uMax;
    float vMin;
    float vMax;
};

auto facePlane(const OMVoxelAABB &box, OMVoxelFacing face) -> FacePlane
{
    switch (face)
    {
    case NegX:
    case PosX:
        return {0, 1, 2, face == PosX, box.offset.y, box.offset.y + box.size.y, box.offset.z,
                box.offset.z + box.size.z, face == PosX ? box.offset.x + box.size.x : box.offset.x};
    case NegY:
    case PosY:
        return {1, 0, 2, face == PosY, box.offset.x, box.offset.x + box.size.x, box.offset.z,
                box.offset.z + box.size.z, face == PosY ? box.offset.y + box.size.y : box.offset.y};
    case NegZ:
    case PosZ:
        return {2, 0, 1, face == PosZ, box.offset.x, box.offset.x + box.size.x, box.offset.y,
                box.offset.y + box.size.y, face == PosZ ? box.offset.z + box.size.z : box.offset.z};
    default:
        return {0, 1, 2, false, 0, 0, 0, 0, 0};
    }
}

auto component(const glm::vec3 &v, int axis) -> float
{
    return axis == 0 ? v.x : (axis == 1 ? v.y : v.z);
}

auto rectCovers(const std::vector<FaceRect> &rects, const FaceRect &target) -> bool
{
    std::vector<float> us = {target.uMin, target.uMax};
    std::vector<float> vs = {target.vMin, target.vMax};

    for (const auto &r : rects)
    {
        us.push_back(r.uMin);
        us.push_back(r.uMax);
        vs.push_back(r.vMin);
        vs.push_back(r.vMax);
    }

    auto uniqueSorted = [](std::vector<float> &values) {
        std::sort(values.begin(), values.end());
        values.erase(std::unique(values.begin(), values.end(),
                                 [](float a, float b) { return std::abs(a - b) <= faceEpsilon; }),
                     values.end());
    };
    uniqueSorted(us);
    uniqueSorted(vs);

    for (size_t ui = 0; ui + 1 < us.size(); ++ui)
    {
        const float u0 = us[ui];
        const float u1 = us[ui + 1];
        if (u1 - u0 <= faceEpsilon)
        {
            continue;
        }
        for (size_t vi = 0; vi + 1 < vs.size(); ++vi)
        {
            const float v0 = vs[vi];
            const float v1 = vs[vi + 1];
            if (v1 - v0 <= faceEpsilon)
            {
                continue;
            }

            const float uc = (u0 + u1) * 0.5f;
            const float vc = (v0 + v1) * 0.5f;
            bool covered = false;
            for (const auto &r : rects)
            {
                if (uc >= r.uMin - faceEpsilon && uc <= r.uMax + faceEpsilon && vc >= r.vMin - faceEpsilon &&
                    vc <= r.vMax + faceEpsilon)
                {
                    covered = true;
                    break;
                }
            }
            if (!covered)
            {
                return false;
            }
        }
    }
    return true;
}
} // namespace

auto OMVoxelCompiler::checkVoxelOcculusion(
    const world::OMChunk<16> &chunk, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
    glm::ivec3 v, OMVoxelFacing face, OMVoxelFacing cullFace, uint32_t id, int pid) -> bool
{
    if (face == None || cullFace == None || pid < 0)
    {
        return false;
    }

    const auto currentModel = modelFetcher(id, chunk.chunkx, chunk.chunky, chunk.chunkz, v.x, v.y, v.z);
    if (handler->queryNumParts(currentModel) <= pid || handler->queryTranslucent(currentModel))
    {
        return false;
    }

    const auto box = handler->queryPartAABB(currentModel, pid);
    const auto planeInfo = facePlane(box, face);
    const FaceRect faceRect{planeInfo.uMin, planeInfo.uMax, planeInfo.vMin, planeInfo.vMax};
    if (faceRect.uMax - faceRect.uMin <= faceEpsilon || faceRect.vMax - faceRect.vMin <= faceEpsilon)
    {
        return false;
    }

    auto rectsFromModel = [&](int modelId, int skipPid, float axisShift) {
        std::vector<FaceRect> rects;
        const int parts = handler->queryNumParts(modelId);
        for (int part = 0; part < parts; ++part)
        {
            if (part == skipPid)
            {
                continue;
            }

            auto partBox = handler->queryPartAABB(modelId, part);
            partBox.offset[planeInfo.axis] += axisShift;
            const float partMin = component(partBox.offset, planeInfo.axis);
            const float partMax = partMin + component(partBox.size, planeInfo.axis);

            // The occluder must start exactly at this face plane and extend to
            // the outside. A part behind the face must not cull it.
            if (planeInfo.positive)
            {
                if (partMin < planeInfo.plane - faceEpsilon || partMin > planeInfo.plane + faceEpsilon)
                {
                    continue;
                }
            }
            else
            {
                if (partMax > planeInfo.plane + faceEpsilon || partMax < planeInfo.plane - faceEpsilon)
                {
                    continue;
                }
            }

            const float u0 = std::max(faceRect.uMin, component(partBox.offset, planeInfo.u));
            const float u1 = std::min(faceRect.uMax, component(partBox.offset, planeInfo.u) +
                                                       component(partBox.size, planeInfo.u));
            const float v0 = std::max(faceRect.vMin, component(partBox.offset, planeInfo.v));
            const float v1 = std::min(faceRect.vMax, component(partBox.offset, planeInfo.v) +
                                                       component(partBox.size, planeInfo.v));
            if (u1 - u0 > faceEpsilon && v1 - v0 > faceEpsilon)
            {
                rects.push_back({u0, u1, v0, v1});
            }
        }
        return rects;
    };

    // Other parts of the same block can cover an internal face, without any
    // block-boundary lookup.
    auto rects = rectsFromModel(currentModel, pid, 0.0f);
    if (rectCovers(rects, faceRect))
    {
        return true;
    }

    const bool boundary = planeInfo.positive ? planeInfo.plane >= 16.0f - faceEpsilon
                                             : planeInfo.plane <= faceEpsilon;
    if (!boundary || face != cullFace)
    {
        return false;
    }

    glm::ivec3 n = v;
    float axisShift = 0.0f;
    switch (cullFace)
    {
    case NegX:
        n.x -= 1;
        axisShift = -16.0f;
        break;
    case NegY:
        n.y -= 1;
        axisShift = -16.0f;
        break;
    case NegZ:
        n.z -= 1;
        axisShift = -16.0f;
        break;
    case PosX:
        n.x += 1;
        axisShift = 16.0f;
        break;
    case PosY:
        n.y += 1;
        axisShift = 16.0f;
        break;
    case PosZ:
        n.z += 1;
        axisShift = 16.0f;
        break;
    default:
        return false;
    }

    const auto neighbourState = queryBlockState(chunk, externalAccessor, n.x, n.y, n.z);
    const auto neighbourModel = modelFetcher(neighbourState, chunk.chunkx, chunk.chunky, chunk.chunkz, n.x, n.y, n.z);

    if (handler->queryTranslucent(neighbourModel) || handler->queryLeaves(neighbourModel))
    {
        return false;
    }

    if (handler->querySoild(neighbourModel))
    {
        return true;
    }

    rects = rectsFromModel(neighbourModel, -1, axisShift);
    return rectCovers(rects, faceRect);
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

        /*if (handler->queryFluid(bsid))
        {
            const auto isFalling = handler->queryFluidFalling(bsid);
            const auto fluidLevel = (handler->queryFluidLevel(bsid) & 0b111);

            auto useSec = fluidLevel != 0 || isFalling;

            const glm::u8vec3 fluidTint =
                paletteProvider ? paletteProvider->fluidColor(chunk, v.first.x, v.first.y, v.first.z, v.second)
                                : glm::u8vec3{255, 255, 255};

            auto vox =
                packVoxelFluid(v.first.x, v.first.y, v.first.z, PosY,
                               useSec ? handler->queryFluidTexSec(bsid) : handler->queryFluidTex(bsid), chunkid, 15, 15,
                               15, 15, 0, 0, 0, 0, 255, 255, 255, 255, useSec, fluidTint.r, fluidTint.g, fluidTint.b);
            (trans || handler->queryWaterlogged(bsid) ? committerTranslucentFluid : committerFluid)(
                OMVoxelFluid{vox[0], vox[1], vox[2], vox[3], vox[4]});
        }*/

        if (handler->queryNumParts(bsid) == 0)
        {
            continue;
        }

        for (int i = 0; i < handler->queryNumParts(bsid); ++i)
        {
            for (auto f : {NegX, NegY, NegZ, PosX, PosY, PosZ})
            {
                const auto cullFace = handler->queryPartFaceCull(bsid, i, f);
                if (handler->queryPartFaceEnabled(bsid, i, f) &&
                    !checkVoxelOcculusion(chunk, externalAccessor, v.first, f, cullFace, v.second, i) &&
                    !checkSkipRendering(chunk, externalAccessor, v.first, cullFace, v.second))
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
