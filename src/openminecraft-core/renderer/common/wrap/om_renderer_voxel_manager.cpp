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
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
uint64_t cx = 0, cy = 0, cz = 0;
OMVoxelManager::OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *target, OMRendererTexture *tex,
                               OMRendererTexture *texSec, std::shared_ptr<world::OMChunkManager<16>> man,
                               std::function<void()> rec, OMVoxelHandler *handler,
                               std::function<uint32_t(uint32_t)> converter)
    : logger("OMVoxelManager", this)
{
    this->rec = rec;
    this->renderer = renderer;
    this->chunkManager = man;
    this->voxelHandler = handler;
    this->converter = converter;

    delete compiler.handler;
    compiler.handler = voxelHandler;
    compiler.converter = converter;

    basics::OMVertexFormat format, format2, formatComplex;
    format.setInstance()
        ->appendPart("voxelPos", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->appendPart("voxelExtra", basics::Integer)
        ->appendPart("voxelExtra2", basics::Integer)
        ->appendPart("voxelExtra3", basics::Integer)
        ->nextGroup()
        ->decideStruct();

    format2.appendPart("voxelPos", basics::Vec3f)->nextGroup()->decideStruct();
    formatComplex.setInstance()
        ->appendPart("voxelBasics", basics::Integer)
        ->appendPart("voxelMetadata", basics::Integer)
        ->appendPart("voxelExtra", basics::Integer)
        ->appendPart("voxelOffset", basics::Vec3f)
        ->appendPart("voxelUV0", basics::Vec2f)
        ->appendPart("voxelUV1", basics::Vec2f)
        ->appendPart("voxelRotationAngle", basics::Float)
        ->appendPart("voxelRotationCenter", basics::Vec3f)
        ->appendPart("voxelSize", basics::Vec3f)
        ->appendPart("voxelRotationAngleExt1", basics::Float)
        ->appendPart("voxelRotationAngleExt2", basics::Float)
        ->nextGroup()
        ->decideStruct();

    pipeline = renderer->createPipeline()
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
                   ->shader(renderer->shaderManager.preprocess("core/voxel.frag.glsl", Fragment, GLSLSource, format))
                   ->shader(renderer->shaderManager.preprocess("core/voxel.vert.glsl", Vertex, GLSLSource, format))
                   ->format(format)
                   ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                   ->blend(true)
                   ->depth(true, true)
                   ->depthEquals(true)
                   ->depthReverseZ(true)
                   ->buildN();

    complexPipeline = renderer->createPipeline()
                          ->input(UniformBuffer)
                          ->inputName("Camera")
                          ->input(ImageSampler)
                          ->inputName("inTexture")
                          ->input(ImageSampler)
                          ->inputName("inTextureSec")
                          ->input(UniformTexelBuffer)
                          ->inputName("inChunkPos")
                          ->output(target)
                          ->samples(4)
                          ->setCullMode(renderer::common::Back)
                          ->setFrontClockwise(true)
                          ->shader(renderer->shaderManager.preprocess("core/voxelcomplex.frag.glsl", Fragment,
                                                                      GLSLSource, formatComplex))
                          ->shader(renderer->shaderManager.preprocess("core/voxelcomplex.vert.glsl", Vertex, GLSLSource,
                                                                      formatComplex))
                          ->format(formatComplex)
                          ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                          ->blend(true)
                          ->depth(true, true)
                          ->depthEquals(true)
                          ->depthReverseZ(true)
                          ->buildN();

    debugPipeline =
        renderer->createPipeline()
            ->input(UniformBuffer)
            ->inputName("Camera")
            ->primitiveType(LineList)
            ->setLineWidth(2.0f)
            ->output(target)
            ->samples(4)
            ->shader(renderer->shaderManager.preprocess("core/voxeldebug.frag.glsl", Fragment, GLSLSource, format2))
            ->shader(renderer->shaderManager.preprocess("core/voxeldebug.vert.glsl", Vertex, GLSLSource, format2))
            ->format(format2)
            ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
            ->blend(true)
            ->depth(true, true)
            ->depthReverseZ(true)
            ->buildN();

    voxels = new OMRendererSegBuf(renderer, 16 * sizeof(OMVoxel));
    voxelsComplex = new OMRendererSegBuf(renderer, 16 * sizeof(OMVoxelComplex));

    chunkBlocks.resize(1);
    chunkBlocksComplex.resize(1);

    chunkoffs = renderer->allocateBuffer(UniformTexel, 3 * sizeof(float));
    debugoffs = renderer->allocateBuffer(VertexData, 12 * 2 * 3 * sizeof(float));

    textureAtlas = tex;
    textureAtlasSecondary = texSec;

    pipeline->bindInput(1, textureAtlas);
    pipeline->bindInput(2, chunkoffs);

    complexPipeline->bindInput(1, textureAtlas);
    complexPipeline->bindInput(2, textureAtlasSecondary);
    complexPipeline->bindInput(3, chunkoffs);
}
OMVoxelManager::~OMVoxelManager()
{
    delete voxels;
    delete voxelsComplex;
    delete chunkoffs;
    delete debugoffs;
    delete pipeline;
    delete complexPipeline;
    delete debugPipeline;
}

auto OMVoxelManager::compile(int i) -> void
{
    auto externalAccessor = [&](glm::ivec3 pos, int64_t chunkx, int64_t chunky, int64_t chunkz) -> uint32_t {
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
        if (chunkManager->chunkLoaded(idx))
        {
            return chunkManager->getChunk(idx).fetch(pos.x, pos.y, pos.z);
        }
        else
        {
            return 0;
        }
    };
    std::vector<OMVoxel> m = {};
    std::vector<OMVoxelComplex> cm = {};

    auto &ck = chunkManager->getChunk(i);
    if (ck.has_value())
    {
        compiler.compile(
            ck.value(), externalAccessor, i, [&](OMVoxel v) -> void { m.emplace_back(v); },
            [&](OMVoxelComplex v) -> void { cm.emplace_back(v); });
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

    if (chunkBlocksComplex.size() <= i)
    {
        chunkBlocksComplex.resize(i + 1);
    }

    while (!chunkBlocksComplex[i].empty())
    {
        voxelsComplex->deallocate(chunkBlocksComplex[i].back());
        chunkBlocksComplex[i].pop_back();
    }

    while (!cm.empty())
    {
        auto blk =
            voxelsComplex->allocate(sizeof(OMVoxelComplex), cm.size() * sizeof(OMVoxelComplex), sizeof(OMVoxelComplex));
        voxelsComplex->update(blk, cm.data());
        cm.erase(cm.begin(), std::next(cm.begin(), blk.length / sizeof(OMVoxelComplex)));
        chunkBlocksComplex[i].push_back(blk);
    }
} // namespace openminecraft::renderer::common::wrap

auto OMVoxelManager::update(basics::OMCamera &camera) -> void
{
    auto cc = camera.getPosRaw();
    auto pp = basics::OMPosition<16, int64_t, float>(cc.chunkx, cc.chunky, cc.chunkz);
    auto pp2 = basics::OMPosition<16, int64_t, float>(cc.chunkx + 1, cc.chunky, cc.chunkz);
    auto pp3 = basics::OMPosition<16, int64_t, float>(cc.chunkx, cc.chunky + 1, cc.chunkz);
    auto pp4 = basics::OMPosition<16, int64_t, float>(cc.chunkx, cc.chunky, cc.chunkz + 1);
    auto pp5 = basics::OMPosition<16, int64_t, float>(cc.chunkx + 1, cc.chunky + 1, cc.chunkz);
    auto pp6 = basics::OMPosition<16, int64_t, float>(cc.chunkx + 1, cc.chunky, cc.chunkz + 1);
    auto pp7 = basics::OMPosition<16, int64_t, float>(cc.chunkx, cc.chunky + 1, cc.chunkz + 1);
    auto pp8 = basics::OMPosition<16, int64_t, float>(cc.chunkx + 1, cc.chunky + 1, cc.chunkz + 1);

    debugoffs->updateData(std::array<glm::vec3, 2 * 12>{
        {
            pp - camera.getPosRaw(),  pp2 - camera.getPosRaw(), pp - camera.getPosRaw(),  pp3 - camera.getPosRaw(),
            pp - camera.getPosRaw(),  pp4 - camera.getPosRaw(), pp8 - camera.getPosRaw(), pp5 - camera.getPosRaw(),
            pp8 - camera.getPosRaw(), pp6 - camera.getPosRaw(), pp8 - camera.getPosRaw(), pp7 - camera.getPosRaw(),
            pp2 - camera.getPosRaw(), pp5 - camera.getPosRaw(), pp2 - camera.getPosRaw(), pp6 - camera.getPosRaw(),
            pp3 - camera.getPosRaw(), pp5 - camera.getPosRaw(), pp3 - camera.getPosRaw(), pp7 - camera.getPosRaw(),
            pp4 - camera.getPosRaw(), pp6 - camera.getPosRaw(), pp4 - camera.getPosRaw(), pp7 - camera.getPosRaw(),
        }}.data());
    if (chunkManager->numChunks())
    {
        std::vector<glm::vec3> offs = {};
        offs.resize(chunkManager->numChunks());
        auto l = voxels->totalSize;
        auto l2 = voxelsComplex->totalSize;
        chunkManager->withChunks([&](std::vector<std::optional<world::OMChunk<16>>> &chunks) -> void {
            int i = 0;
            for (auto &ochk : chunks)
            {
                if (!ochk.has_value())
                {
                    compile(i);
                    offs[i] = glm::vec3{INFINITY};
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
                    offs[i] = pp - camera.getPosRaw();
                }
                ++i;
            }
        });

        if (offs.size() * sizeof(glm::vec3) > chunkoffs->length)
        {
            delete chunkoffs;
            chunkoffs = renderer->allocateBuffer(UniformTexel, chunkManager->numChunks() * 3 * sizeof(float) * 2);
            chunkoffs->updateData(offs.data());
            pipeline->bindInput(2, chunkoffs);
            complexPipeline->bindInput(3, chunkoffs);
        }
        else
        {
            chunkoffs->updateData(offs.data());
        }

        if (l != voxels->totalSize || l2 != voxelsComplex->totalSize)
        {
            rec();
        }
    }
}

auto OMVoxelManager::submit(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(pipeline)
        ->vertexBuffer({voxels->buffer})
        ->drawInstanceN(6, voxels->totalSize / sizeof(OMVoxel))
        ->pipeline(complexPipeline)
        ->vertexBuffer({voxelsComplex->buffer})
        ->drawInstanceN(6, voxelsComplex->totalSize / sizeof(OMVoxelComplex))
        ->pipeline(debugPipeline)
        ->vertexBuffer({debugoffs})
        ->drawN(2 * 12);
}
} // namespace openminecraft::renderer::common::wrap
