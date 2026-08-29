#ifndef OM_RENDEER_VOXEL_HPP
#define OM_RENDEER_VOXEL_HPP

#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
struct OMVoxelAABB
{
    glm::vec3 offset;
    glm::vec3 size;

    [[nodiscard]] inline auto applyOffset(glm::vec3 v) const -> OMVoxelAABB
    {
        return {offset + v, size};
    }

    [[nodiscard]] inline auto intersect(const OMVoxelAABB &other) const -> bool
    {
        glm::vec3 currMin = offset;
        glm::vec3 currMax = currMin + size;
        glm::vec3 tgtMin = other.offset;
        glm::vec3 tgtMax = tgtMin + other.size;
        return currMin.x <= tgtMax.x && currMax.x >= tgtMin.x && currMin.y <= tgtMax.y && currMax.y >= tgtMin.y &&
               currMin.z <= tgtMax.z && currMax.z >= tgtMin.z;
    }

    [[nodiscard]] inline auto contains(const glm::vec3 p) const -> bool
    {
        glm::vec3 currMin = offset;
        glm::vec3 currMax = currMin + size;
        return currMin.x <= p.x && currMax.x >= p.x && currMin.y <= p.y && currMax.y >= p.y && currMin.z <= p.z &&
               currMax.z >= p.z;
    }
};
struct OMVoxelShape
{
    std::vector<OMVoxelAABB> aabbs = {};

  public:
    OMVoxelShape() = default;
    OMVoxelShape(const OMVoxelAABB &a)
    {
        aabbs.emplace_back(a);
    }
    OMVoxelShape(const std::initializer_list<OMVoxelAABB> l) : aabbs(l)
    {
    }
    OMVoxelShape(const std::vector<OMVoxelAABB> l) : aabbs(l)
    {
    }

    [[nodiscard]] inline auto contains(const glm::vec3 p) const -> bool
    {
        for (auto const &a : aabbs)
        {
            if (a.contains(p))
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] inline auto intersect(const OMVoxelAABB &other) const -> bool
    {
        for (auto const &a : aabbs)
        {
            if (other.intersect(a))
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] inline auto intersect(const OMVoxelShape &other) const -> bool
    {
        for (auto const &a : aabbs)
        {
            if (other.intersect(a))
            {
                return true;
            }
        }

        return false;
    }
};
struct OMVoxel
{
    int32_t voxelBasic;
    int32_t voxelMeta;
    int32_t voxelExtra;
    int32_t voxelExtra2;
    int32_t voxelExtra3;
};
struct OMVoxelComplex
{
    int32_t voxelBasic;
    int32_t voxelMeta;
    int32_t voxelExtra;
    glm::vec3 voxelOffset;
    glm::vec2 voxelUV0, voxelUV1;
    float voxelRotationAngle;
    glm::vec3 voxelRotationCenter, voxelSize;
    float voxelRotationAngleExt1, voxelRotationAngleExt2;
};
enum OMVoxelFacing : uint8_t
{
    NegX = 0b000,
    PosX = 0b100,
    NegZ = 0b001,
    PosZ = 0b101,
    NegY = 0b010,
    PosY = 0b110,
    None = 0b111,
};
// INFO: note for the ambient occulusion
// Type -> (AO1, AO2, AO3, AO4)
// NegX => ((0, 1, 0), (0, 0, 0), (0, 1, 1), (0, 0, 1))
// PosX => ((1, 1, 1), (1, 0, 1), (1, 1, 0), (1, 0, 0))
// NegZ => ((1, 1, 0), (1, 0, 0), (0, 1, 0), (0, 0, 0))
// PosZ => ((0, 1, 1), (0, 0, 1), (1, 1, 1), (1, 0, 1))
// NegY => ((0, 0, 0), (0, 0, 1), (1, 0, 0), (1, 0, 1))
// PosY => ((0, 1, 0), (0, 1, 1), (1, 1, 0), (1, 1, 1))

class OMVoxelHandler
{
  public:
    virtual ~OMVoxelHandler()
    {
    }
    virtual auto queryNumParts(int bsid) -> int = 0;
    virtual auto queryPartFaceEnabled(int bsid, int pid, OMVoxelFacing) -> bool = 0;
    virtual auto queryPartFaceTex(int bsid, int pid, OMVoxelFacing) -> int = 0;
    virtual auto queryPartFaceCull(int bsid, int pid, OMVoxelFacing) -> OMVoxelFacing = 0;
    virtual auto queryPartAABB(int bsid, int pid) -> OMVoxelAABB = 0;
    virtual auto queryPartFaceUV(int bsid, int pid, OMVoxelFacing) -> glm::vec4 = 0;
    virtual auto queryPartFaceRotation(int bsid, int pid, OMVoxelFacing) -> int = 0;
    virtual auto queryPartRotationAxis(int bsid, int pid) -> int = 0;
    virtual auto queryPartRotationCenter(int bsid, int pid) -> glm::vec3 = 0;
    virtual auto queryPartRotationAngle(int bsid, int pid) -> int = 0;
    virtual auto queryPartRotationAngleExt(int bsid, int pid) -> glm::vec3 = 0;
    virtual auto querySoild(int bsid) -> bool = 0;
    virtual auto queryOcclusionShape(int bsid) -> OMVoxelShape = 0;
    virtual auto queryPartAmbientOcclusion(int bsid, int pid) -> bool = 0;
    virtual auto queryPartShade(int bsid, int pid) -> bool = 0;
    virtual auto queryPartComplex(int bsid, int pid) -> bool = 0;
    virtual auto queryPartFaceSecondaryTexture(int bsid, int pid, OMVoxelFacing) -> bool = 0;
    virtual auto queryPartRotationAngleF(int bsid, int pid) -> float = 0;
    virtual auto queryTranslucent(int bsid) -> bool = 0;
};

class OMVoxelHandlerDummy : public OMVoxelHandler
{
  public:
    OMVoxelHandlerDummy() = default;
    ~OMVoxelHandlerDummy() = default;
    auto queryNumParts(int b) -> int override
    {
        return b == 0 ? 0 : 1;
    }
    auto queryPartFaceEnabled(int b, int, OMVoxelFacing) -> bool override
    {
        return b != 0;
    }
    auto queryPartFaceTex(int b, int, OMVoxelFacing) -> int override
    {
        return b;
    }
    auto queryPartFaceCull(int, int, OMVoxelFacing v) -> OMVoxelFacing override
    {
        return v;
    }
    auto queryPartAABB(int bsid, int pid) -> OMVoxelAABB override
    {
        return {{0, 0, 0}, {16, 16, 16}};
    }
    auto queryPartFaceUV(int bsid, int pid, OMVoxelFacing) -> glm::vec4 override
    {
        return {0, 0, 16, 16};
    }
    auto queryPartFaceRotation(int bsid, int pid, OMVoxelFacing) -> int override
    {
        return 0;
    }
    auto queryPartRotationAxis(int bsid, int pid) -> int override
    {
        return 0;
    }
    auto queryPartRotationCenter(int bsid, int pid) -> glm::vec3 override
    {
        return {0, 0, 0};
    }
    auto queryPartRotationAngle(int bsid, int pid) -> int override
    {
        return 2;
    }
    auto querySoild(int bsid) -> bool override
    {
        return true;
    }
    auto queryOcclusionShape(int bsid) -> OMVoxelShape override
    {
        return {{{0, 0, 0}, {16, 16, 16}}};
    }
    auto queryPartAmbientOcclusion(int bsid, int pid) -> bool override
    {
        return true;
    }
    auto queryPartShade(int bsid, int pid) -> bool override
    {
        return true;
    }
    auto queryPartComplex(int bsid, int pid) -> bool override
    {
        return false;
    }
    auto queryPartFaceSecondaryTexture(int bsid, int pid, OMVoxelFacing) -> bool override
    {
        return false;
    }
    auto queryPartRotationAngleF(int bsid, int pid) -> float override
    {
        return 0.0f;
    }
    auto queryPartRotationAngleExt(int bsid, int pid) -> glm::vec3 override
    {
        return {};
    }
    auto queryTranslucent(int bsid) -> bool override
    {
        return false;
    }
};

class OMVoxelCompiler
{
  public:
    OMVoxelCompiler();
    ~OMVoxelCompiler();

    auto queryBlockstate(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                         int x, int y, int z) -> uint32_t;
    auto existSoild(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>, int x,
                    int y, int z) -> bool;
    auto computeAO(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>, int x,
                   int y, int z, OMVoxelFacing, int bsid, int pid) -> std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>;
    auto checkExistSoild(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                         glm::ivec3, OMVoxelFacing) -> bool;
    auto compile(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                 int chunkid, std::function<void(OMVoxel)>, std::function<void(OMVoxelComplex)>,
                 std::function<void(OMVoxel)>, std::function<void(OMVoxelComplex)>) -> void;

    OMVoxelHandler *handler = new OMVoxelHandlerDummy();
    std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)> converter;

  private:
    log::OMLogger logger;
};

template <typename It> class OMVoxelLayer
{
  public:
    OMVoxelLayer(OMRenderer *renderer)
    {
        voxelBuffer = new OMRendererSegBuf(renderer, 16 * sizeof(It));
        chunkBlocks.resize(1);
    }
    ~OMVoxelLayer()
    {
        delete voxelBuffer;
    }

    inline void loadData(int i, std::vector<It> &cm)
    {
        if (chunkBlocks.size() <= i)
        {
            chunkBlocks.resize(i + 1);
        }

        while (!chunkBlocks[i].empty())
        {
            voxelBuffer->deallocate(chunkBlocks[i].back());
            chunkBlocks[i].pop_back();
        }

        while (!cm.empty())
        {
            auto blk = voxelBuffer->allocate(sizeof(It), cm.size() * sizeof(It), sizeof(It));
            voxelBuffer->update(blk, cm.data());
            cm.erase(cm.begin(), std::next(cm.begin(), blk.length / sizeof(It)));
            chunkBlocks[i].push_back(blk);
        }
    }

    auto buf() -> OMRendererSegBuf *
    {
        return voxelBuffer;
    }

  private:
    std::vector<std::list<OMRendererSegBufBlock>> chunkBlocks;
    OMRendererSegBuf *voxelBuffer;
};

struct OMVoxelSkyDisc
{
    glm::vec3 diskCenterColor;
    float discRange;
    glm::vec3 diskSideColor;
    float discHeight;
};

class OMVoxelCompilerPool
{
  public:
    OMVoxelCompilerPool(world::OMChunkManager<16> &, OMVoxelCompiler &, int = 4);
    ~OMVoxelCompilerPool();

    void upload(OMVoxelLayer<OMVoxel> *cutout, OMVoxelLayer<OMVoxelComplex> *cutoutComplex,
                OMVoxelLayer<OMVoxel> *translucent, OMVoxelLayer<OMVoxelComplex> *translucentComplex);
    void compile(int i);

  private:
    world::OMChunkManager<16> &manager;
    OMVoxelCompiler &compiler;

    std::mutex poolMutex;
    std::vector<int> queuedChunks;

    std::mutex bufferMutex;
    std::unordered_map<int, std::vector<OMVoxel>> cutout, translucent;
    std::unordered_map<int, std::vector<OMVoxelComplex>> cutoutComplex, translucentComplex;

    std::vector<std::thread *> thrs;
    bool active = true;
};

class OMVoxelManager
{
  public:
    OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *, OMRendererTexture *, OMRendererTexture *,
                   std::shared_ptr<world::OMChunkManager<16>>, std::function<void()>, OMVoxelHandler *,
                   std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)>);
    ~OMVoxelManager();

    auto submit(OMRendererTask *, OMRendererTempTarget *) -> OMRendererTask *;
    auto update(basics::OMCamera &camera) -> void;
    void bindCameraBuffer(OMRendererBuffer *);

    OMRendererTempTarget *translucentTargetMS, *translucentTarget;
    OMRendererTempTarget *cutoutTargetMS, *cutoutTarget;
    OMRendererPipeline *pipeline, *debugPipeline, *complexPipeline;
    OMRendererPipeline *translucentPipeline, *translucentComplexPipeline;
    OMRendererPipeline *composePipeline;
    OMRendererPipeline *skyDiscPipeline;
    OMRendererBuffer *chunkoffs, *debugoffs;
    OMRendererBuffer *skydisc;
    OMRendererTexture *textureAtlas;
    OMRendererTexture *textureAtlasSecondary;

  private:
    int samples = 4;
    OMVoxelHandler *voxelHandler;
    OMRenderer *renderer;
    std::function<void()> rec;
    log::OMLogger logger;
    OMVoxelCompiler compiler;

    std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)> converter;
    std::shared_ptr<world::OMChunkManager<16>> chunkManager;
    OMVoxelLayer<OMVoxel> *voxelLayer, *voxelTranslucentLayer;
    OMVoxelLayer<OMVoxelComplex> *voxelComplexLayer, *voxelTranslucentComplexLayer;

    OMVoxelCompilerPool *compilerPool;

    void unloadChunk(int i);
};

} // namespace openminecraft::renderer::common::wrap

#endif
