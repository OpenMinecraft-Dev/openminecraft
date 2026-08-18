#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
uint64_t cx = 0, cy = 0, cz = 0;
OMVoxelManager::OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *target, OMRendererTexture *tex,
                               std::shared_ptr<world::OMChunkManager<16>> man, std::function<void()> rec)
    : logger("OMVoxelManager", this)
{
    this->rec = rec;
    this->renderer = renderer;
    this->chunkManager = man;

    basics::OMVertexFormat format;
    format.setInstance()
        ->appendPart("voxelPos", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->appendPart("voxelExtra", basics::Integer)
        ->appendPart("voxelExtra2", basics::Integer)
        ->nextGroup()
        ->decideStruct();
    auto voxelFrg = renderer->shaderManager.preprocess("core/voxel.frag.glsl", Fragment, GLSLSource, format);
    auto voxelVtx = renderer->shaderManager.preprocess("core/voxel.vert.glsl", Vertex, GLSLSource, format);

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->inputName("ObjectInfo")
                   ->input(UniformBuffer)
                   ->inputName("Camera")
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

    voxels = new OMRendererSegBuf(renderer, 0x200);

    chunkBlocks.resize(1);

    chunkoffs = renderer->allocateBuffer(UniformTexel, 3 * sizeof(float));

    textureAtlas = tex;

    pipeline->bindInput(2, textureAtlas);
    pipeline->bindInput(3, chunkoffs);
}
OMVoxelManager::~OMVoxelManager()
{
    delete voxels;
    delete chunkoffs;
    delete pipeline;
}

auto OMVoxelManager::compile(int i) -> void
{
    auto externalAccessor = [&](glm::ivec3 pos, int64_t chunkx, int64_t chunky, int64_t chunkz) -> bool {
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

        world::OMChunkIndex idx = {chunkx, chunky, chunkz};

        return chunkManager->chunkLoaded(idx) && chunkManager->getChunk(idx).exists(pos.x, pos.y, pos.z);
    };
    std::vector<OMVoxel> m = {};
    auto &ck = chunkManager->getChunk(i);
    if (ck.has_value())
    {
        compiler.compile(ck.value(), externalAccessor, i, [&](OMVoxel v) { m.emplace_back(v); });
    }

    if (chunkBlocks.size() <= i)
    {
        chunkBlocks.resize(i + 1);
    }

    while (!chunkBlocks[i].empty())
    {
        voxels->deallocate(chunkBlocks[i].back());
        chunkBlocks[i].pop_back();
    }

    while (!m.empty())
    {
        auto blk = voxels->allocate(sizeof(OMVoxel), m.size() * sizeof(OMVoxel), sizeof(OMVoxel));
        voxels->update(blk, m.data());
        m.erase(m.begin(), std::next(m.begin(), blk.length / sizeof(OMVoxel)));
        chunkBlocks[i].push_back(blk);
    }
}

auto OMVoxelManager::update(basics::OMCamera &camera) -> void
{
    if (chunkManager->numChunks())
    {
        std::vector<glm::vec3> offs = {};
        offs.reserve(chunkManager->numChunks());
        auto l = voxels->totalSize;
        chunkManager->withChunks([&](std::vector<std::optional<world::OMChunk<16>>> &chunks) {
            int i = 0;
            for (auto &ochk : chunks)
            {
                if (!ochk.has_value())
                {
                    compile(i);
                    offs.emplace_back(INFINITY);
                }
                else
                {
                    auto &chk = ochk.value();
                    if (chk.isDirty())
                    {
                        compile(i);
                        chk.solveDirty();
                    }
                    auto pp = basics::OMPosition<16, int64_t, float>();
                    pp.chunkx = chk.chunkx;
                    pp.chunky = chk.chunky;
                    pp.chunkz = chk.chunkz;
                    offs.emplace_back(pp - camera.getPosRaw());
                }
                ++i;
            }
        });

        if (offs.size() * sizeof(glm::vec3) > chunkoffs->length)
        {
            delete chunkoffs;
            chunkoffs = renderer->allocateBuffer(UniformTexel, chunkManager->numChunks() * 3 * sizeof(float) * 2);
            pipeline->bindInput(3, chunkoffs);
        }
        chunkoffs->updateData(offs.data());

        if (l != voxels->totalSize)
        {
            rec();
        }
    }
}

auto OMVoxelManager::submit(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(pipeline)
        ->vertexBuffer({voxels->buffer})
        ->drawInstanceN(6, voxels->totalSize / sizeof(OMVoxel));
}
} // namespace openminecraft::renderer::common::wrap
