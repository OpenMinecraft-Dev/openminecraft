#ifndef OM_RENDERER_VOXEL_HPP
#define OM_RENDERER_VOXEL_HPP

#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_object_ref.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <list>
#include <mutex>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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
    int32_t voxelType;
};
struct OMVoxelFluid
{
    int32_t voxelBasic;
    int32_t voxelMeta;
    int32_t voxelExtra;
    int32_t voxelExtra2;
    int32_t voxelType;
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
    int32_t voxelType;
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
    virtual auto querySkipsRendering(int bsid, int tgbsid, OMVoxelFacing) -> bool = 0;
    virtual auto queryNumParts(int bsid) -> int = 0;
    virtual auto queryPartFaceEnabled(int bsid, int pid, OMVoxelFacing) -> bool = 0;
    virtual auto queryPartFaceTex(int bsid, int pid, OMVoxelFacing) -> int = 0;
    virtual auto queryPartFaceCull(int bsid, int pid, OMVoxelFacing) -> OMVoxelFacing = 0;
    virtual auto queryPartAABB(int bsid, int pid) -> OMVoxelAABB = 0;
    virtual auto queryPartFaceUV(int bsid, int pid, OMVoxelFacing) -> glm::vec4 = 0;
    virtual auto queryPartFaceRotation(int bsid, int pid, OMVoxelFacing) -> int = 0;
    virtual auto queryPartFaceTint(int bsid, int pid, OMVoxelFacing) -> int = 0;
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
    virtual auto queryFluid(int bsid) -> bool = 0;
    virtual auto queryFluidFalling(int bsid) -> bool = 0;
    virtual auto queryFluidLevel(int bsid) -> uint8_t = 0;
    virtual auto queryFluidTex(int bsid) -> int = 0;
    virtual auto queryWaterlogged(int bsid) -> bool = 0;
    virtual auto queryFluidSame(int bsid1, int bsid2) -> bool = 0;
    virtual auto queryFluidTexSec(int bsid) -> int = 0;
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
    auto queryPartFaceTint(int bsid, int pid, OMVoxelFacing) -> int override
    {
        return -1;
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
    auto querySkipsRendering(int bsid, int tgbsid, OMVoxelFacing) -> bool override
    {
        return false;
    }
    auto queryFluid(int bsid) -> bool override
    {
        return false;
    }
    auto queryFluidFalling(int bsid) -> bool override
    {
        return false;
    }
    auto queryFluidLevel(int bsid) -> uint8_t override
    {
        return 0;
    }
    auto queryFluidTex(int bsid) -> int override
    {
        return 0;
    }
    auto queryWaterlogged(int bsid) -> bool override
    {
        return false;
    }
    auto queryFluidSame(int bsid1, int bsid2) -> bool override
    {
        return true;
    }
    auto queryFluidTexSec(int bsid) -> int override
    {
        return 0;
    }
};

class OMChunkPaletteProvider
{
  public:
    virtual ~OMChunkPaletteProvider() = default;

    virtual auto color(const world::OMChunk<16> &chunk, int x, int y, int z, uint32_t blockState, int tintIndex)
        -> glm::u8vec3 = 0;

    // Fluid tinting: water inside waterlogged blocks should use the water color,
    // not the block's own colorSource (usually None -> white)
    virtual auto fluidColor(const world::OMChunk<16> &chunk, int x, int y, int z, uint32_t blockState) -> glm::u8vec3
    {
        return color(chunk, x, y, z, blockState, 0);
    }
};

// Compute a 6x6 face visibility matrix from the 16x16x16 opaque occupancy grid (bit63 = known)
auto computeSectionVisibility(const std::array<uint8_t, 4096> &blocking) -> uint64_t;

class OMVoxelCompiler
{
  public:
    OMVoxelCompiler();
    ~OMVoxelCompiler();

    auto queryBlockState(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                         int x, int y, int z) -> uint32_t;
    auto existSoild(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>, int x,
                    int y, int z) -> bool;
    auto existFluidSame(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                        int x, int y, int z, int) -> bool;
    auto computeAO(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>, int x,
                   int y, int z, OMVoxelFacing, int bsid, int pid) -> std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>;
    auto checkExistSoild(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                         glm::ivec3, OMVoxelFacing) -> bool;
    auto checkExistFluidSame(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                             glm::ivec3, OMVoxelFacing, int) -> bool;
    auto checkSkip(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                   glm::ivec3, OMVoxelFacing, uint32_t) -> bool;
    auto checkAvgFluid(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                       glm::ivec3, int, int) -> float;
    auto compile(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>,
                 int chunkid, std::function<void(OMVoxel)>, std::function<void(OMVoxelComplex)>,
                 std::function<void(OMVoxelFluid)>, std::function<void(OMVoxel)>, std::function<void(OMVoxelComplex)>,
                 std::function<void(OMVoxelFluid)>) -> void;

    // Visibility-only scan (no mesh generation): fill visibility data for all chunks used by occlusion-culling BFS
    auto computeVisibility(const world::OMChunk<16> &, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)>)
        -> uint64_t;

    OMVoxelHandler *handler = new OMVoxelHandlerDummy();
    std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)> modelFetcher;

    std::shared_ptr<OMChunkPaletteProvider> paletteProvider = nullptr;

  private:
    log::OMLogger logger;
};

template <typename It> class OMVoxelLayer
{
  public:
    OMVoxelLayer(OMRenderer *renderer, std::string tag)
    {
        voxelBuffer = new OMRendererSegBuf(renderer, std::move(tag), 1024 * 1024);
        chunkBlocks.resize(1);
    }
    ~OMVoxelLayer()
    {
        delete voxelBuffer;
    }

    inline void loadData(int i, std::vector<It> &cm, bool flush = false)
    {
        if (chunkBlocks.size() <= i)
        {
            chunkBlocks.resize(i + 1);
        }

        while (!chunkBlocks[i].empty())
        {
            liveBytes -= chunkBlocks[i].back().length;
            voxelBuffer->deallocate(chunkBlocks[i].back());
            chunkBlocks[i].pop_back();
        }

        while (!cm.empty())
        {
            auto blk = voxelBuffer->allocate(sizeof(It), cm.size() * sizeof(It), sizeof(It));
            voxelBuffer->update(blk, cm.data());
            cm.erase(cm.begin(), std::next(cm.begin(), blk.length / sizeof(It)));
            liveBytes += blk.length;
            chunkBlocks[i].push_back(blk);
        }

        if (flush)
        {
            voxelBuffer->flushDirty();
        }
    }

    auto merge()
    {
        auto mergeStart = std::chrono::steady_clock::now();

        for (auto &p : chunkBlocks)
        {
            p = voxelBuffer->compact(p);
        }
    }

    // Time-limited compaction: avoid compacting an entire layer at once and causing tens of ms of stutter
    auto mergeStepTimed(double budgetMs) -> void
    {
        if (chunkBlocks.empty())
        {
            return;
        }

        const auto start = std::chrono::steady_clock::now();
        size_t processed = 0;

        while (mergeCursor < chunkBlocks.size())
        {
            chunkBlocks[mergeCursor] = voxelBuffer->compact(chunkBlocks[mergeCursor]);
            ++mergeCursor;
            ++processed;

            if ((processed & 31) == 0 &&
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() >
                    budgetMs)
            {
                break;
            }
        }

        if (mergeCursor >= chunkBlocks.size())
        {
            mergeCursor = 0;
        }
    }

    auto mergeStep(size_t maxChunks) -> void
    {
        if (chunkBlocks.empty())
        {
            return;
        }

        size_t processed = 0;

        while (processed < maxChunks && mergeCursor < chunkBlocks.size())
        {
            chunkBlocks[mergeCursor] = voxelBuffer->compact(chunkBlocks[mergeCursor]);
            ++mergeCursor;
            ++processed;
        }

        if (mergeCursor >= chunkBlocks.size())
        {
            mergeCursor = 0;
        }
    }

    auto buf() -> OMRendererSegBuf *
    {
        return voxelBuffer;
    }


    // Actual live data size (used to judge fragmentation)
    auto liveBytesCount() const -> uint64_t
    {
        return liveBytes;
    }

  private:
    size_t mergeCursor = 0;
    uint64_t liveBytes = 0;
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

struct OMVoxelSunrise
{
    glm::vec4 sunColor;
    float sunAngle;
};

struct OMVoxelMoon
{
    float moonAngle;
    int moonPhase;
};

struct OMVoxelCloud
{
    glm::vec3 camera;
    float opacity;
    glm::vec3 color;
};

struct OMVoxelLightMap
{
    float skyFactor;
    float blockFactor;
    float nightVisionFactor;
    float darknessScale;
    float bossOverlayWorldDarkeningFactor;
    float brightnessFactor;
    float pad01, pad02;
    glm::vec3 blockLightTint;
    float pad1 = 0;
    glm::vec3 skyLightColor;
    float pad2 = 0;
    glm::vec3 ambientColor;
    float pad3 = 0;
    glm::vec3 nightVisionColor;
    float pad4 = 0;
};

class OMVoxelCompilerPool
{
  public:
    OMVoxelCompilerPool(world::OMChunkManager<16> &, OMVoxelCompiler &, int = 4);
    ~OMVoxelCompilerPool();

    void upload(OMVoxelLayer<OMVoxel> *cutout, OMVoxelLayer<OMVoxelComplex> *cutoutComplex,
                OMVoxelLayer<OMVoxelFluid> *fluid, OMVoxelLayer<OMVoxel> *translucent,
                OMVoxelLayer<OMVoxelComplex> *translucentComplex, OMVoxelLayer<OMVoxelFluid> *translucentFluid,
                const std::function<void(int, world::OMChunkIndex, uint64_t)> &visSink);
    void compile(int i, bool useCache = false, float distanceSq = 3.4e38f);
    // Lightweight visibility request (no geometry; only fills data for occlusion culling)
    void requestVisibility(int i);
    void dropCache(int i)
    {
        std::lock_guard g(bufferMutex);
        cached.erase(i);
        cachedOrigin.erase(i);
        cutoutC.erase(i);
        cutoutComplexC.erase(i);
        translucentC.erase(i);
        translucentComplexC.erase(i);
        visC.erase(i);
        visPending.erase(i);
    }

  private:
    std::unordered_map<int, bool> cached;
    // Cache the chunk coordinates: slots are recycled by chunkManager,
    // so the cache is valid only when coordinates match
    std::unordered_map<int, world::OMChunkIndex> cachedOrigin;
    world::OMChunkManager<16> &manager;
    OMVoxelCompiler &compiler;

    std::mutex poolMutex;

    // Compile requests: sorted by squared distance to the camera (nearest first)
    struct OMCompileRequest
    {
        float distanceSq = 0.0f;
        int chunk = -1;
        bool useCache = false;

        bool operator>(const OMCompileRequest &other) const
        {
            return distanceSq > other.distanceSq;
        }
    };

    std::priority_queue<OMCompileRequest, std::vector<OMCompileRequest>, std::greater<>> queuedChunks;
    std::vector<int> visRequests;
    std::unordered_set<int> visQueued;
    // Keep one entry per chunk in the queue (fast rotation repeatedly requests the same chunk)
    std::unordered_set<int> queuedIndex;

    std::mutex bufferMutex;
    std::unordered_map<int, std::vector<OMVoxel>> cutout, translucent;
    std::unordered_map<int, std::vector<OMVoxelComplex>> cutoutComplex, translucentComplex;
    std::unordered_map<int, std::vector<OMVoxelFluid>> fluid, translucentFluid;
    // Per-section 6x6 face visibility matrix: bit(f*6 + t) means you can see in through face f and exit through face t
    std::unordered_map<int, uint64_t> visC;
    std::unordered_map<int, std::pair<world::OMChunkIndex, uint64_t>> visPending;

    std::unordered_map<int, std::vector<OMVoxel>> cutoutC, translucentC;
    std::unordered_map<int, std::vector<OMVoxelComplex>> cutoutComplexC, translucentComplexC;
    std::unordered_map<int, std::vector<OMVoxelFluid>> fluidC, translucentFluidC;

    std::vector<std::thread *> thrs;
    bool active = true;
};

class OMVoxelColorManager
{
  public:
    virtual ~OMVoxelColorManager()
    {
    }

    virtual auto getSkyDiscColor() -> glm::vec3 = 0;
    virtual auto getSkyDiskRange() -> float = 0;
    virtual auto getSkyDiskHeight() -> float = 0;
    virtual auto getFogRange() -> glm::vec2 = 0;
    virtual auto getBlockTint() -> glm::vec3 = 0;
    virtual auto getSkyLightColor() -> glm::vec3 = 0;
    virtual auto getAmbientColor() -> glm::vec3 = 0;
    virtual auto getNightVisionColor() -> glm::vec3 = 0;
    virtual auto getBlockFactor() -> float = 0;
    virtual auto getSkyFactor() -> float = 0;
    virtual auto getNightVisionFactor() -> float = 0;
    virtual auto getDarknessScale() -> float = 0;
    virtual auto getBossOverlayWorldDarkeningFactor() -> float = 0;
    virtual auto getBrightnessFactor() -> float = 0;
    virtual auto getFogColor() -> glm::vec3 = 0;
    virtual auto getSunriseColor() -> glm::vec4 = 0;
    virtual auto getSunAngle() -> float = 0;
    virtual auto getMoonAngle() -> float = 0;
    virtual auto getMoonPhase() -> int = 0;
    virtual auto getStarOpacity() -> float = 0;
    virtual auto getStarRotation() -> float = 0;
    virtual auto getCloudColor() -> glm::vec4 = 0;

    inline auto isDirty() -> bool
    {
        return dirty;
    }
    inline void solveDirty()
    {
        dirty = false;
    }

  protected:
    std::atomic_bool dirty = true;
};

class OMVoxelManager
{
  public:
    OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *, const std::string &texTag,
                   const std::string &texSecTag, std::shared_ptr<world::OMChunkManager<16>>,
                   std::function<void(bool)>, OMVoxelHandler *,
                   std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)>,
                   OMVoxelColorManager *, const std::string &sunTag, const std::string &moonTag,
                   const std::string &cloudTag, std::vector<bool>, std::shared_ptr<OMChunkPaletteProvider>);
    ~OMVoxelManager();

    auto submit(OMRendererTask *, OMRendererTempTarget *, bool = true) -> OMRendererTask *;
    auto update(basics::OMCamera &camera) -> void;

    auto updateColor() -> void;
    auto calcActualFogColor() -> glm::vec3;
    void bindCameraBuffer(OMRendererBuffer *);
    auto buildVoxelCloud() -> std::vector<uint32_t>;
    auto hashLayerState() -> std::array<uint32_t, 12>;
    auto fetchLayerState() -> std::array<std::tuple<uint32_t, uint32_t, uint32_t>, 6>;
    auto fetchChunkState() -> std::pair<uint32_t, uint32_t>;

    OMRendererTempTarget *translucentTargetMS, *translucentTarget;
    OMRendererTempTarget *cutoutTargetMS, *cutoutTarget;
    OMRendererTempTarget *cloudTargetMS, *cloudTarget;
    OMRendererPipeline *pipeline, *debugPipeline, *complexPipeline, *fluidPipeline;
    OMRendererPipeline *translucentPipeline, *translucentComplexPipeline, *translucentFluidPipeline;
    OMRendererPipeline *composePipeline;
    OMRendererPipeline *skyDiscPipeline;
    OMRendererPipeline *lightmapPipeline;
    OMRendererPipeline *skyPipeline;
    OMBufferRef chunkOffsetBuffer, debugOffsetBuffer;
    // Shared index buffer for voxel quads (0,1,2, 2,3,0): 4 vertices per face
    OMBufferRef voxelIndexBuffer;
    OMBufferRef skydisc;
    OMBufferRef fogdata, cloudFogdata;
    OMBufferRef lightmapData;
    OMBufferRef moonData;
    OMBufferRef cloudData;
    OMTextureRef cloudTex;
    OMTextureRef textureAtlas;
    OMTextureRef textureAtlasSecondary;
    OMRendererTempTarget *lightmap;
    OMVoxelColorManager *colorManager;

    OMBufferRef sunrise;
    OMRendererPipeline *sunrisePipeline;
    OMRendererPipeline *sunPipeline, *moonPipeline;

    OMTextureRef sunTex, moonTex;
    OMBufferRef starBuffer;
    OMBufferRef starBaseData;
    OMRendererPipeline *starPipeline;

    OMRendererPipeline *cloudPipeline, *cloudComposePipeline;
    std::vector<bool> cloudStats;
    OMBufferRef cloudBuffer;

  private:
    int samples = 4;
    OMVoxelHandler *voxelHandler;
    OMRenderer *renderer;
    std::function<void(bool)> recordTask;
    log::OMLogger logger;
    OMVoxelCompiler compiler;
    std::shared_ptr<OMChunkPaletteProvider> paletteProvider = nullptr;

    std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)> modelFetcher;
    std::shared_ptr<world::OMChunkManager<16>> chunkManager;
    OMVoxelLayer<OMVoxel> *voxelLayer, *voxelTranslucentLayer;
    OMVoxelLayer<OMVoxelComplex> *voxelComplexLayer, *voxelTranslucentComplexLayer;
    OMVoxelLayer<OMVoxelFluid> *voxelFluidLayer, *voxelTranslucentFluidLayer;

    OMVoxelCompilerPool *compilerPool;

    glm::vec3 actualFogColor = glm::vec3(1.0f);
    glm::vec3 cameraForward = glm::vec3(0.0f, 0.0f, 1.0f);
    int renderDistance = 8;

    std::vector<glm::vec4> chunkOffsetCache;

    std::array<int64_t, 3> lastCameraChunk = {INT64_MIN, INT64_MIN, INT64_MIN};
    glm::vec3 lastCameraLocal = glm::vec3{INFINITY};
    size_t lastOffsetCount = 0;
    bool offsetUploadedOnce = false;
    std::vector<uint16_t> hideFrames;
    static constexpr uint16_t unloadDelay = 30;

    float lastCameraYaw = INFINITY;
    float lastCameraPitch = INFINITY;
    uint64_t lastLoadVersion = 0;
    uint32_t pendingUnload = 0;
    bool cullingReady = false;
    std::vector<int> cullCandidates;

    std::vector<uint64_t> sectionVisibility;
    // Visibility prefetch cursor (rotates over cullCandidates, at most 48 per frame)
    size_t visFillCursor = 0;
    std::vector<uint8_t> bfsVisibleMask;
    uint64_t graphVersion = 0;
    uint64_t bfsGraphVersion = ~0ull;
    std::array<int64_t, 3> lastBfsSection = {INT64_MIN, INT64_MIN, INT64_MIN};
    std::chrono::steady_clock::time_point lastBfsTime = std::chrono::steady_clock::now();
    bool bfsValid = false;
    // Whether clouds are visible (layer invisible when pitch + half FOV is below the horizon; the pass can be skipped)
    bool renderClouds = true;
    glm::vec2 lastExtent = glm::vec2(0.0f);

    void unloadChunk(int i);
};

} // namespace openminecraft::renderer::common::wrap

#endif
