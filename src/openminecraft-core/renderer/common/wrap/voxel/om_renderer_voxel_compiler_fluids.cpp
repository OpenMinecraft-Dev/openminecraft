#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_compiler.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_objects.hpp"
#include <array>
#include <tuple>
#include <utility>

namespace openminecraft::renderer::common::wrap::voxel
{
auto OMVoxelCompiler::checkFluidState(const world::OMChunk<16> &chunk,
                                      std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                      glm::ivec3 pos, uint32_t id)
    -> std::tuple<std::array<uint8_t, 4>, std::vector<OMVoxelFacing>>
{
    constexpr std::array<std::pair<OMVoxelFacing, glm::ivec3>, 6> states = {{
        {PosX, {1, 0, 0}},
        {NegX, {-1, 0, 0}},
        {PosY, {0, 1, 0}},
        {NegY, {0, -1, 0}},
        {PosZ, {0, 0, 1}},
        {NegZ, {0, 0, -1}},
    }};

    std::vector<OMVoxelFacing> fac;
    for (auto s : states)
    {
        auto p = pos + s.second;
        auto next = externalAccessor(p, chunk.chunkx, chunk.chunky, chunk.chunkz);
        auto nextModel = modelFetcher(next, chunk.chunkx, chunk.chunky, chunk.chunkz, p.x, p.y, p.z);

        if (handler->querySoild(nextModel))
        {
            continue;
        }
        if (!handler->queryFluid(nextModel) || !handler->queryFluidSame(id, nextModel))
        {
            fac.emplace_back(s.first);
        }
    }

    return {{255, 255, 255, 255}, fac};
}
} // namespace openminecraft::renderer::common::wrap::voxel