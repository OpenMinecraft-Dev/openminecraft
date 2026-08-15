#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"

namespace openminecraft::renderer::common::wrap
{
OMVoxelCompiler::OMVoxelCompiler() : logger("OMVoxelCompiler", this)
{
}
OMVoxelCompiler::~OMVoxelCompiler()
{
}

// INFO: packed vertex structure in u32
// xxxx yyyy zzzz ssss bbbb efff aaaaaaaa
static auto packVoxelPos(uint8_t x, uint8_t y, uint8_t z, uint8_t skyLight, uint8_t blockLight, OMVoxelFacing facing,
                         uint8_t ao1, uint8_t ao2, uint8_t ao3, uint8_t ao4) -> int
{
    return x << 28 | y << 24 | z << 20 | skyLight << 16 | blockLight << 12 | 1 << 11 | facing << 8 | ao1 << 6 |
           ao2 << 4 | ao3 << 2 | ao4;
}
// INFO: packed vertex metadata in u32
// DS*16 CS*16
static auto packVoxelMetadata(uint16_t tex, uint16_t chkid) -> int
{
    return tex << 16 | chkid;
}

auto OMVoxelCompiler::compile(world::OMChunk<16> &chunk,
                              std::function<bool(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor, int chunkid)
    -> std::vector<int>
{
    std::vector<int> d = {};

    auto exist = [&](int x, int y, int z) -> bool {
        if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
            return externalAccessor(glm::ivec3(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz);
        return chunk.exists(x, y, z);
    };

    auto computeAO = [&](int x, int y, int z, OMVoxelFacing facing) -> std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> {
        uint8_t ao1 = 0, ao2 = 0, ao3 = 0, ao4 = 0;

        auto countNeighbors = [&](int dx, int dy, int dz) -> int { return exist(x + dx, y + dy, z + dz) ? 1 : 0; };

        switch (facing)
        {
        case OMVoxelFacing::NegY: {
            ao1 = countNeighbors(-1, 0, -1) + countNeighbors(-1, 0, 0) + countNeighbors(0, 0, -1);
            ao2 = countNeighbors(-1, 0, 1) + countNeighbors(-1, 0, 0) + countNeighbors(0, 0, 1);
            ao3 = countNeighbors(1, 0, -1) + countNeighbors(1, 0, 0) + countNeighbors(0, 0, -1);
            ao4 = countNeighbors(1, 0, 1) + countNeighbors(1, 0, 0) + countNeighbors(0, 0, 1);
            break;
        }
        case OMVoxelFacing::PosY: {
            ao1 = countNeighbors(-1, 0, -1) + countNeighbors(-1, 0, 0) + countNeighbors(0, 0, -1);
            ao2 = countNeighbors(-1, 0, 1) + countNeighbors(-1, 0, 0) + countNeighbors(0, 0, 1);
            ao3 = countNeighbors(1, 0, -1) + countNeighbors(1, 0, 0) + countNeighbors(0, 0, -1);
            ao4 = countNeighbors(1, 0, 1) + countNeighbors(1, 0, 0) + countNeighbors(0, 0, 1);
            break;
        }
        case OMVoxelFacing::NegX: {
            ao2 = countNeighbors(0, -1, -1) + countNeighbors(0, -1, 0) + countNeighbors(0, 0, -1);
            ao1 = countNeighbors(0, 1, -1) + countNeighbors(0, 1, 0) + countNeighbors(0, 0, -1);
            ao4 = countNeighbors(0, -1, 1) + countNeighbors(0, -1, 0) + countNeighbors(0, 0, 1);
            ao3 = countNeighbors(0, 1, 1) + countNeighbors(0, 1, 0) + countNeighbors(0, 0, 1);
            break;
        }
        case OMVoxelFacing::PosX: {
            ao4 = countNeighbors(0, -1, -1) + countNeighbors(0, -1, 0) + countNeighbors(0, 0, -1);
            ao3 = countNeighbors(0, 1, -1) + countNeighbors(0, 1, 0) + countNeighbors(0, 0, -1);
            ao2 = countNeighbors(0, -1, 1) + countNeighbors(0, -1, 0) + countNeighbors(0, 0, 1);
            ao1 = countNeighbors(0, 1, 1) + countNeighbors(0, 1, 0) + countNeighbors(0, 0, 1);
            break;
        }
        case OMVoxelFacing::NegZ: {
            ao4 = countNeighbors(-1, -1, 0) + countNeighbors(-1, 0, 0) + countNeighbors(0, -1, 0);
            ao3 = countNeighbors(-1, 1, 0) + countNeighbors(-1, 0, 0) + countNeighbors(0, 1, 0);
            ao2 = countNeighbors(1, -1, 0) + countNeighbors(1, 0, 0) + countNeighbors(0, -1, 0);
            ao1 = countNeighbors(1, 1, 0) + countNeighbors(1, 0, 0) + countNeighbors(0, 1, 0);
            break;
        }
        case OMVoxelFacing::PosZ: {
            ao2 = countNeighbors(-1, -1, 0) + countNeighbors(-1, 0, 0) + countNeighbors(0, -1, 0);
            ao1 = countNeighbors(-1, 1, 0) + countNeighbors(-1, 0, 0) + countNeighbors(0, 1, 0);
            ao4 = countNeighbors(1, -1, 0) + countNeighbors(1, 0, 0) + countNeighbors(0, -1, 0);
            ao3 = countNeighbors(1, 1, 0) + countNeighbors(1, 0, 0) + countNeighbors(0, 1, 0);
            break;
        }
        default:
            break;
        }
        return {ao1, ao2, ao3, ao4};
    };

    for (auto const &v : chunk)
    {
        if (v.second == 0)
        {
            continue;
        }

        if (!exist(v.first.x, v.first.y - 1, v.first.z))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.first.x, v.first.y, v.first.z, OMVoxelFacing::NegY);
            d.emplace_back(
                packVoxelPos(v.first.x, v.first.y, v.first.z, 15, 0, OMVoxelFacing::NegY, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.second, chunkid));
        }

        if (!exist(v.first.x, v.first.y + 1, v.first.z))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.first.x, v.first.y, v.first.z, OMVoxelFacing::PosY);
            d.emplace_back(
                packVoxelPos(v.first.x, v.first.y, v.first.z, 15, 0, OMVoxelFacing::PosY, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.second, chunkid));
        }

        if (!exist(v.first.x - 1, v.first.y, v.first.z))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.first.x, v.first.y, v.first.z, OMVoxelFacing::NegX);
            d.emplace_back(
                packVoxelPos(v.first.x, v.first.y, v.first.z, 15, 0, OMVoxelFacing::NegX, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.second, chunkid));
        }

        if (!exist(v.first.x + 1, v.first.y, v.first.z))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.first.x, v.first.y, v.first.z, OMVoxelFacing::PosX);
            d.emplace_back(
                packVoxelPos(v.first.x, v.first.y, v.first.z, 15, 0, OMVoxelFacing::PosX, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.second, chunkid));
        }

        if (!exist(v.first.x, v.first.y, v.first.z - 1))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.first.x, v.first.y, v.first.z, OMVoxelFacing::NegZ);
            d.emplace_back(
                packVoxelPos(v.first.x, v.first.y, v.first.z, 15, 0, OMVoxelFacing::NegZ, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.second, chunkid));
        }

        if (!exist(v.first.x, v.first.y, v.first.z + 1))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.first.x, v.first.y, v.first.z, OMVoxelFacing::PosZ);
            d.emplace_back(
                packVoxelPos(v.first.x, v.first.y, v.first.z, 15, 0, OMVoxelFacing::PosZ, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.second, chunkid));
        }
    }

    return d;
}
} // namespace openminecraft::renderer::common::wrap