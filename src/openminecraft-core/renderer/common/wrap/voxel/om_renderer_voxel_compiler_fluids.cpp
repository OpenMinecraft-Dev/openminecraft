#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_compiler.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_objects.hpp"
#include <array>
#include <cmath>
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
        if (s.first == PosY)
        {
            continue;
        }
        auto p = pos + s.second;
        auto next = externalAccessor(p, chunk.chunkx, chunk.chunky, chunk.chunkz);
        auto nextModel =
            modelFetcher ? modelFetcher(next, chunk.chunkx, chunk.chunky, chunk.chunkz, p.x, p.y, p.z) : next;

        if (handler->querySoild(nextModel))
        {
            continue;
        }
        if (handler->queryFluid(nextModel) && handler->queryFluidSame(id, nextModel))
        {
            continue;
        }
        fac.emplace_back(s.first);
    }

    auto modelAt = [&](glm::ivec3 p) -> uint32_t {
        uint32_t state = 0;
        if (p.x >= 0 && p.x < 16 && p.y >= 0 && p.y < 16 && p.z >= 0 && p.z < 16)
        {
            state = chunk.fetch(p.x, p.y, p.z);
        }
        else
        {
            state = externalAccessor(p, chunk.chunkx, chunk.chunky, chunk.chunkz);
        }
        return modelFetcher ? modelFetcher(state, chunk.chunkx, chunk.chunky, chunk.chunkz, p.x, p.y, p.z) : state;
    };

    auto blockHeight = [&](glm::ivec3 p) -> float {
        const uint32_t model = modelAt(p);
        if (!handler->queryFluid(model))
        {
            return 0.0f;
        }

        const uint32_t above = modelAt(p + glm::ivec3{0, 1, 0});
        if (handler->queryFluid(above) && handler->queryFluidSame(model, above))
        {
            return 1.0f;
        }

        if (handler->queryFluidFalling(model))
        {
            return 1.0f;
        }

        const auto level = handler->queryFluidLevel(model);
        if (level != 0)
        {
            return std::clamp((8.0f - static_cast<float>(level)) / 9.0f, 0.0f, 1.0f);
        }

        bool full = handler->querySoild(above);
        if (full)
        {
            constexpr std::array<glm::ivec3, 4> around = {glm::ivec3{1, 1, 0}, glm::ivec3{-1, 1, 0},
                                                          glm::ivec3{0, 1, 1}, glm::ivec3{0, 1, -1}};
            for (const auto &offset : around)
            {
                if (!handler->querySoild(modelAt(p + offset)) && !handler->queryFluid(modelAt(p + offset)))
                {
                    full = false;
                    break;
                }
            }
        }
        return full ? 1.0f : 8.0f / 9.0f;
    };

    constexpr std::array<std::array<glm::ivec3, 4>, 4> cornerSamples = {{
        {{{-1, 0, -1}, {-1, 0, 0}, {0, 0, -1}, {0, 0, 0}}},
        {{{-1, 0, 1}, {-1, 0, 0}, {0, 0, 1}, {0, 0, 0}}},
        {{{1, 0, -1}, {1, 0, 0}, {0, 0, -1}, {0, 0, 0}}},
        {{{1, 0, 1}, {1, 0, 0}, {0, 0, 1}, {0, 0, 0}}},
    }};

    const float currentHeight = blockHeight(pos);

    std::array<uint8_t, 4> heights = {};
    for (size_t corner = 0; corner < cornerSamples.size(); ++corner)
    {
        float sum = 0.0f;
        bool fullCorner = false;
        for (const auto &offset : cornerSamples[corner])
        {
            const auto samplePos = pos + offset;
            const auto sampleModel = modelAt(samplePos);
            if (handler->queryFluid(sampleModel))
            {
                if (handler->queryFluidSame(id, sampleModel))
                {
                    const float sampleHeight = blockHeight(samplePos);
                    sum += sampleHeight;
                    if (sampleHeight >= 0.999f)
                    {
                        fullCorner = true;
                    }
                }
                // A different fluid contributes zero.
            }
            else
            {
                // Non-fluid neighbours keep the current fluid height instead
                // of pulling the corner down to zero.
                sum += currentHeight;
            }
        }
        if (fullCorner)
        {
            heights[corner] = 255;
            continue;
        }
        const float average = sum / 4.0f;
        heights[corner] = static_cast<uint8_t>(std::clamp(std::lround(average * 255.0f), 0L, 255L));
    }

    auto isForcedFull = [&](glm::ivec3 p) -> bool {
        if (!handler->querySoild(modelAt(p + glm::ivec3{0, 1, 0})))
        {
            return false;
        }
        constexpr std::array<glm::ivec3, 4> around = {glm::ivec3{1, 1, 0}, glm::ivec3{-1, 1, 0}, glm::ivec3{0, 1, 1},
                                                      glm::ivec3{0, 1, -1}};
        for (const auto &offset : around)
        {
            if (!handler->querySoild(modelAt(p + offset)))
            {
                return false;
            }
        }
        return true;
    };

    // Emit the top face only for an actual fluid surface: no solid ceiling,
    // no same-fluid block above. A forced-full source has no visible top.
    if (!isForcedFull(pos))
    {
        const auto aboveModel = modelAt(pos + glm::ivec3{0, 1, 0});
        if (!handler->queryFluid(aboveModel) || !handler->queryFluidSame(id, aboveModel))
        {
            fac.emplace_back(PosY);
        }
    }

    return {heights, fac};
}
} // namespace openminecraft::renderer::common::wrap::voxel