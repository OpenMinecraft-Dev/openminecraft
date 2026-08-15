#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
OMVoxelManager::OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *target) : logger("OMVoxelManager", this)
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
    for (int cx = 0; cx < 8; ++cx)
    {
        for (int cy = 0; cy < 8; ++cy)
        {
            for (int cz = 0; cz < 8; ++cz)
            {
                world::OMChunk<16> datar(cx, cy, cz);
                for (int x = 0; x < 16; ++x)
                {
                    for (int y = 0; y < 16; ++y)
                    {
                        for (int z = 0; z < 16; ++z)
                        {
                            datar.setBlock(x, y, z, (rand() % 8));
                        }
                    }
                }
                chunks.emplace_back(datar);
            }
        }
    }

    auto externalAccessor = [&](glm::ivec3 pos, int64_t chunkx, int64_t chunky, int64_t chunkz) -> bool {
        /*logger.info("External Accessor called for ({}, {}, {}) at ({}, {}, {})", pos.x, pos.y, pos.z, chunkx, chunky,
                    chunkz);*/
        if (pos.x < 0)
        {
            chunkx -= 1;
            pos.x += 16;
        }
        else if (pos.x >= 16)
        {
            chunkx += 1;
            pos.x -= 16;
        }
        if (pos.y < 0)
        {
            chunky -= 1;
            pos.y += 16;
        }
        else if (pos.y >= 16)
        {
            chunky += 1;
            pos.y -= 16;
        }
        if (pos.z < 0)
        {
            chunkz -= 1;
            pos.z += 16;
        }
        else if (pos.z >= 16)
        {
            chunkz += 1;
            pos.z -= 16;
        }

        for (auto &chk : chunks)
        {
            if (chk.chunkx == chunkx && chk.chunky == chunky && chk.chunkz == chunkz)
            {
                return chk.exists(pos.x, pos.y, pos.z);
            }
        }
        return false;
    };

    std::vector<int> data = {};

    int cid = 0;
    for (auto &chk : chunks)
    {
        auto r = compiler.compile(chk, externalAccessor, cid);
        data.insert(data.end(), r.begin(), r.end());
        ++cid;
    }

    voxels = renderer->allocateBuffer(InstanceData, data.size() * sizeof(uint32_t));
    voxels->updateData(data.data());
    faceCount = data.size();

    chunkoffs = renderer->allocateBuffer(UniformTexel, chunks.size() * 3 * sizeof(float));

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
    return task->pipeline(pipeline)->vertexBuffer({voxels})->drawInstanceN(6, faceCount / 2);
}

auto OMVoxelManager::update(basics::OMCamera &camera) -> void
{
    int i = 0;
    for (auto &chk : chunks)
    {
        auto pp = basics::OMPosition<16, int64_t, float>();
        pp.chunkx = chk.chunkx;
        pp.chunky = chk.chunky;
        pp.chunkz = chk.chunkz;
        auto l = pp - camera.getPosRaw();
        chunkoffs->updateDataPart(&l, sizeof(glm::vec3) * i, sizeof(glm::vec3));
        ++i;
    }
}
} // namespace openminecraft::renderer::common::wrap
