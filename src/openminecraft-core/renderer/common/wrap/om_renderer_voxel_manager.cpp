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
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
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
        ->appendPart("voxelExtra", basics::Integer)
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
    for (int cx = 0; cx < 4; ++cx)
    {
        for (int cy = 0; cy < 4; ++cy)
        {
            for (int cz = 0; cz < 4; ++cz)
            {
                world::OMChunk<16> datar(cx, cy, cz);
                for (int x = 0; x < 16; ++x)
                {
                    for (int y = 0; y < 16; ++y)
                    {
                        for (int z = 0; z < 16; ++z)
                        {
                            datar.setBlock(x, y, z, (rand() % 7) + 1);
                        }
                    }
                }
                chunks.emplace_back(datar);
            }
        }
    }

    voxels = new OMRendererSegBuf(renderer, 0x200);

    {
        chunkBlocks.resize(chunks.size());
        for (int i = 0; i < chunks.size(); ++i)
        {
            compile(i);
        }
    }

    faceCount = voxels->totalSize / sizeof(OMVoxel);

    chunkoffs = renderer->allocateBuffer(UniformTexel, chunks.size() * 3 * sizeof(float));

    textureAtlas = renderer->allocateTexture(16, 16, 8, 4, OMTextureType::Dim2Array, OMTextureArrangement::ColorRgba);
    int i = 0;
    for (auto l : {"dirt", "stone", "cobblestone", "coal_ore", "iron_ore", "dirt", "copper_ore", "diamond_ore"})
    {
        auto imgraw = vfs::fsfetch(fmt::format("/bootassets/external/minecraft/textures/block/{}.png", l));
        specs::png::OMPngFile img2;
        img2.parse(imgraw);
        textureAtlas->updateData(img2.fetchData(), i);
        ++i;
    }
    textureAtlas->mipFilter = Nearest;
    textureAtlas->magFilter = Nearest;
    textureAtlas->minFilter = Nearest;
    textureAtlas->setupSampler();

    pipeline->bindInput(3, textureAtlas);
    pipeline->bindInput(4, chunkoffs);
}
OMVoxelManager::~OMVoxelManager()
{
    delete voxels;
    delete chunkoffs;
    delete textureAtlas;
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

        for (auto &chk : chunks)
        {
            if (chk.chunkx == chunkx && chk.chunky == chunky && chk.chunkz == chunkz)
            {
                return chk.exists(pos.x, pos.y, pos.z);
            }
        }
        return false;
    };
    std::vector<OMVoxel> m = {};
    compiler.compile(chunks[i], externalAccessor, i, [&](OMVoxel v) { m.emplace_back(v); });

    while (!chunkBlocks[i].empty())
    {
        voxels->deallocate(chunkBlocks[i].back());
        chunkBlocks[i].pop_back();
    }

    while (!m.empty())
    {
        auto blk = voxels->allocate(sizeof(OMVoxel), m.size() * sizeof(OMVoxel));
        voxels->update(blk, m.data());
        m.erase(m.begin(), std::next(m.begin(), blk.length / sizeof(OMVoxel)));
        chunkBlocks[i].push_back(blk);
    }
}

auto OMVoxelManager::submit(OMRendererTask *task) -> OMRendererTask *
{
    return task->pipeline(pipeline)->vertexBuffer({voxels->buffer})->drawInstanceN(6, faceCount);
}

auto OMVoxelManager::update(basics::OMCamera &camera) -> void
{
    int i = 0;
    std::vector<glm::vec3> offs = {};
    offs.resize(chunks.size());
    for (auto &chk : chunks)
    {
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
        ++i;
    }
    chunkoffs->updateData(offs.data());
}
} // namespace openminecraft::renderer::common::wrap
