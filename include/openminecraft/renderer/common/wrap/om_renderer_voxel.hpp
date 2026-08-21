#ifndef OM_RENDEER_VOXEL_HPP
#define OM_RENDEER_VOXEL_HPP

#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/world/om_world_chunk.hpp"
#include "openminecraft/world/om_world_chunkmanager.hpp"
#include <cstdint>
#include <initializer_list>
#include <list>
#include <vector>

namespace openminecraft::renderer::common::wrap
{
struct OMVoxelAABB
{
    glm::ivec3 offset;
    glm::ivec3 size;

    [[nodiscard]] inline auto applyOffset(glm::ivec3 v) const -> OMVoxelAABB
    {
        return {offset + v, size};
    }

    [[nodiscard]] inline auto intersect(const OMVoxelAABB &other) const -> bool
    {
        glm::ivec3 currMin = offset;
        glm::ivec3 currMax = currMin + size;
        glm::ivec3 tgtMin = other.offset;
        glm::ivec3 tgtMax = tgtMin + other.size;
        return currMin.x <= tgtMax.x && currMax.x >= tgtMin.x && currMin.y <= tgtMax.y && currMax.y >= tgtMin.y &&
               currMin.z <= tgtMax.z && currMax.z >= tgtMin.z;
    }

    [[nodiscard]] inline auto contains(const glm::ivec3 p) const -> bool
    {
        glm::ivec3 currMin = offset;
        glm::ivec3 currMax = currMin + size;
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

    [[nodiscard]] inline auto contains(const glm::ivec3 p) const -> bool
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
    virtual auto queryPartFaceUV(int bsid, int pid, OMVoxelFacing) -> glm::ivec4 = 0;
    virtual auto queryPartFaceRotation(int bsid, int pid, OMVoxelFacing) -> int = 0;
    virtual auto queryPartRotationAxis(int bsid, int pid) -> int = 0;
    virtual auto queryPartRotationCenter(int bsid, int pid) -> glm::ivec3 = 0;
    virtual auto queryPartRotationAngle(int bsid, int pid) -> int = 0;
    virtual auto querySoild(int bsid) -> bool = 0;
    virtual auto queryOcculusionShape(int bsid) -> OMVoxelShape = 0;
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
    auto queryPartFaceUV(int bsid, int pid, OMVoxelFacing) -> glm::ivec4 override
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
    auto queryPartRotationCenter(int bsid, int pid) -> glm::ivec3 override
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
    auto queryOcculusionShape(int bsid) -> OMVoxelShape override
    {
        return {{{0, 0, 0}, {16, 16, 16}}};
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
                 int chunkid, std::function<void(OMVoxel)>) -> void;

    OMVoxelHandler *handler = new OMVoxelHandlerDummy();

  private:
    log::OMLogger logger;
};

class OMVoxelManager
{
  public:
    OMVoxelManager(OMRenderer *renderer, OMRendererRenderTarget *, OMRendererTexture *,
                   std::shared_ptr<world::OMChunkManager<16>>, std::function<void()>, OMVoxelHandler *);
    ~OMVoxelManager();

    auto submit(OMRendererTask *) -> OMRendererTask *;
    auto update(basics::OMCamera &camera) -> void;

    OMRendererPipeline *pipeline, *debugPipeline;
    OMRendererSegBuf *voxels;
    OMRendererBuffer *chunkoffs, *debugoffs;
    OMRendererTexture *textureAtlas;

  private:
    OMVoxelHandler *voxelHandler;
    OMRenderer *renderer;
    std::function<void()> rec;
    log::OMLogger logger;
    OMVoxelCompiler compiler;

    std::shared_ptr<world::OMChunkManager<16>> chunkManager;
    std::vector<std::list<OMRendererSegBufBlock>> chunkBlocks = {};

    void compile(int i);
};

} // namespace openminecraft::renderer::common::wrap

#endif
