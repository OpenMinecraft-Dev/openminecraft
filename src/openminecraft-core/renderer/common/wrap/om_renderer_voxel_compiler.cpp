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
// s -> Voxel Scale (3 * 4 bits)
// e -> Voxel Enable (1 bit)
// a -> Voxel Ambient Occclusion Levels (4 * 2 bits)
// t -> Voxel Texture Index (16 bits)
// c -> Voxel Chunk ID (16 bits)
// u -> unused
// INFO: packed vertex structure in u32
// xxxx yyyy zzzz efff XXXX YYYY ZZZZ ssss
static auto packVoxelPos(uint8_t x, uint8_t y, uint8_t z, OMVoxelFacing facing, uint8_t dx, uint8_t dy, uint8_t dz,
                         uint8_t scaleY) -> int
{
    return x << 28 | y << 24 | z << 20 | 1 << 19 | (facing & 7) << 16 | dx << 12 | dy << 8 | dz << 4 |
           ((scaleY - 1) & 0xf);
}
// INFO: packed vertex metadata in u32
// tttt tttt tttt tttt cccc cccc cccc cccc
static auto packVoxelMetadata(uint16_t tex, uint16_t chkid) -> int
{
    return (tex << 16) | chkid;
}

// INFO: packed vertex extra in u32
// llll llll llll llll ssss ssss aaaa aaaa
static auto packVoxelExtra(uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t ao1, uint8_t ao2, uint8_t ao3,
                           uint8_t ao4, uint8_t scaleX, uint8_t scaleZ) -> int
{
    return ((l1 & 0xf) << 28) | ((l2 & 0xf) << 24) | ((l3 & 0xf) << 20) | ((l4 & 0xf) << 16) |
           (((scaleX - 1) & 0xf) << 12) | (((scaleZ - 1) & 0xf) << 8) | ao1 << 6 | ao2 << 4 | ao3 << 2 | ao4;
}

constexpr std::array<std::pair<glm::ivec3, OMVoxelFacing>, 6> faceMapping = {{
    {{0, -1, 0}, NegY},
    {{0, 1, 0}, PosY},
    {{-1, 0, 0}, NegX},
    {{1, 0, 0}, PosX},
    {{0, 0, -1}, NegZ},
    {{0, 0, 1}, PosZ},
}};

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

    for (const auto &v : chunk)
    {
        if (v.second == 0)
        {
            continue;
        }

        for (auto &p : faceMapping)
        {
            if (!exist(v.first.x + p.first.x, v.first.y + p.first.y, v.first.z + p.first.z))
            {
                auto [ao1, ao2, ao3, ao4] = computeAO(v.first.x, v.first.y, v.first.z, p.second);

                commiter(OMVoxel{packVoxelPos(v.first.x, v.first.y, v.first.z, p.second, 8, 8, 8, 16),
                                 packVoxelMetadata(v.second, chunkid),
                                 packVoxelExtra(15, 15, 15, 15, ao1, ao2, ao3, ao4, 16, 16)});
            }
        }
    }
}
} // namespace openminecraft::renderer::common::wrap