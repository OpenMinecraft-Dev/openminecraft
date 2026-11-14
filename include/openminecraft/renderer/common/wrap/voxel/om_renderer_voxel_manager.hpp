#ifndef OM_RENDERER_VOXEL_MANAGER_HPP
#define OM_RENDERER_VOXEL_MANAGER_HPP

#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_object_ref.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_compiler_pool.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_objects.hpp"

namespace openminecraft::renderer::common::wrap::voxel
{
class OMVoxelManager
{
  public:
    OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *, const std::string &texTag,
                   const std::string &texSecTag, std::shared_ptr<world::OMChunkManager<16>>, std::function<void(bool)>,
                   OMVoxelHandler *, std::function<uint32_t(uint32_t, uint64_t, uint64_t, uint64_t, int, int, int)>,
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
    // Geometry stays resident while a section is out of view. The offset is
    // moved to infinity instead, which the voxel vertex shaders use to reject
    // the instance before rasterization.
    bool offsetCacheDirty = false;

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

} // namespace openminecraft::renderer::common::wrap::voxel

#endif
