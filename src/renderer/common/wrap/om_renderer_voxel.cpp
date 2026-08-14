#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <array>
#include <bitset>
#include <cstdint>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
// INFO: packed vertex structure in u32
// xxxx yyyy zzzz ssss bbbb ffff a1*2 a2*2 a3*2 a4*2
static auto packVoxelPos(uint8_t x, uint8_t y, uint8_t z, uint8_t skyLight, uint8_t blockLight, OMVoxelFacing facing,
                         uint8_t ao1, uint8_t ao2, uint8_t ao3, uint8_t ao4) -> int
{
    return x << 28 | y << 24 | z << 20 | skyLight << 16 | blockLight << 12 | facing << 8 | ao1 << 6 | ao2 << 4 |
           ao3 << 2 | ao4;
}
// INFO: packed vertex metadata in u32
// DS*16 CS*16
static auto packVoxelMetadata(uint16_t tex, uint16_t chkid) -> int
{
    return tex << 16 | chkid;
}
auto OMVoxelManager::compile(std::vector<OMVoxel> &lb) -> std::vector<int>
{
    std::vector<int> d;

    d.reserve(lb.size() * 12);

    std::bitset<16 * 16 * 16> exists = {false};

    for (auto &v : lb)
    {
        exists.set(v.y * 256 + v.x * 16 + v.z);
    }

    auto exist = [&](int x, int y, int z) -> bool {
        if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
            return false;
        return exists.test(y * 256 + x * 16 + z);
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

    for (auto &v : lb)
    {
        // NegY
        if (!exist(v.x, v.y - 1, v.z))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.x, v.y, v.z, OMVoxelFacing::NegY);
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::NegY, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        // PosY
        if (!exist(v.x, v.y + 1, v.z))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.x, v.y, v.z, OMVoxelFacing::PosY);
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::PosY, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        // NegX
        if (!exist(v.x - 1, v.y, v.z))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.x, v.y, v.z, OMVoxelFacing::NegX);
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::NegX, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        // PosX
        if (!exist(v.x + 1, v.y, v.z))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.x, v.y, v.z, OMVoxelFacing::PosX);
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::PosX, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        // NegZ
        if (!exist(v.x, v.y, v.z - 1))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.x, v.y, v.z, OMVoxelFacing::NegZ);
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::NegZ, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
        // PosZ
        if (!exist(v.x, v.y, v.z + 1))
        {
            auto [ao1, ao2, ao3, ao4] = computeAO(v.x, v.y, v.z, OMVoxelFacing::PosZ);
            d.emplace_back(packVoxelPos(v.x, v.y, v.z, 15, 0, OMVoxelFacing::PosZ, ao1, ao2, ao3, ao4));
            d.emplace_back(packVoxelMetadata(v.texId, 0));
        }
    }

    return d;
}
OMVoxelManager::OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *target)
{
    basics::OMVertexFormat format;
    format.setInstance()
        ->appendPart("voxelPos", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->nextGroup()
        ->decideStruct();
    auto voxelFrg = renderer->shaderManager.preprocess("core/voxel.frag.glsl", Fragment, GLSLSource, format);
    auto voxelVtx = renderer->shaderManager.preprocess("core/voxel.vert.glsl", Vertex, GLSLSource, format);

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->inputName("ObjectInfo")
                   ->input(UniformBuffer)
                   ->inputName("Camera")
                   ->input(UniformBuffer)
                   ->inputName("Lighting")
                   ->input(ImageSampler)
                   ->inputName("inTexture")
                   ->input(UniformTexelBuffer)
                   ->inputName("inChunkPos")
                   ->output(target)
                   ->samples(4)
                   ->setCullMode(renderer::common::Back)
                   ->setFrontClockwise(true)
                   ->shader(voxelFrg)
                   ->shader(voxelVtx)
                   ->format(format)
                   ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                   ->blend(true)
                   ->depth(true, true)
                   ->depthReverseZ(true)
                   ->buildN();

    srand(time(nullptr));
    std::vector<OMVoxel> datar = {};
    for (int x = 0; x < 16; ++x)
    {
        for (int y = 0; y < 16; ++y)
        {
            for (int z = 0; z < 16; ++z)
            {
                OMVoxel v = {rand() % 4, x, y, z};
                datar.emplace_back(v);
            }
        }
    }
    auto data = compile(datar);

    voxels = renderer->allocateBuffer(InstanceData, data.size() * sizeof(uint32_t));
    voxels->updateData(data.data());
    c = data.size();

    chunkoffs = renderer->allocateBuffer(UniformTexel, 3 * sizeof(float));

    pipeline->bindInput(4, chunkoffs);
}
OMVoxelManager::~OMVoxelManager()
{
    delete voxels;
    delete chunkoffs;
    delete pipeline;
}

auto OMVoxelManager::submit(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(pipeline)->vertexBuffer({voxels})->drawInstanceN(6, c / 2);
}

auto OMVoxelManager::update(basics::OMCamera &camera) -> void
{
    auto l = basics::OMPosition<16, int64_t, float>() - camera.getPosRaw();
    chunkoffs->updateData(&l);
}
} // namespace openminecraft::renderer::common::wrap
