#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_compiler.hpp"

namespace openminecraft::renderer::common::wrap::voxel
{
auto OMVoxelCompiler::queryBlockState(const world::OMChunk<16> &chunk,
                                      std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
                                      int x, int y, int z) -> uint32_t
{
    if (x < 0 || y < 0 || z < 0 || x > 15 || y > 15 || z > 15)
    {
        return externalAccessor(glm::ivec3(x, y, z), chunk.chunkx, chunk.chunky, chunk.chunkz);
    }

    return chunk.fetch(x, y, z);
}

auto OMVoxelCompiler::computeAO(const world::OMChunk<16> &chunk,
                                std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor, int x,
                                int y, int z, OMVoxelFacing facing, int modelId, int pid)
    -> std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>
{
    uint8_t ao1 = 0, ao2 = 0, ao3 = 0, ao4 = 0;
    auto currentAabb = handler->queryPartAABB(modelId, pid);
    auto countNeighborsPoint = [&](int dx, int dy, int dz, glm::ivec3 pos) -> int {
        auto tgbs = queryBlockState(chunk, externalAccessor, x + dx, y + dy, z + dz);
        if (tgbs == airId.load())
        {
            return 0;
        }

        auto currentPos =
            glm::mix(currentAabb.offset, currentAabb.offset + currentAabb.size, glm::vec3(pos.x, pos.y, pos.z)) -
            (glm::vec3(dx, dy, dz) * 16.0f);

        auto occ = handler->queryOcclusionShape(modelFetcher(tgbs, chunk.chunkx, chunk.chunky, chunk.chunkz, x, y, z));
        if (occ.contains(currentPos))
        {
            return 1;
        }

        return 0;
    };
    auto finalAO = [&](int corner, int side1, int side2) {
        if (side1 && side2)
        {
            return 3;
        }
        return side1 + side2 + corner;
    };

    switch (facing)
    {
    case OMVoxelFacing::NegY: {
        ao1 = finalAO(countNeighborsPoint(-1, -1, -1, {0, 0, 0}), countNeighborsPoint(-1, -1, 0, {0, 0, 0}),
                      countNeighborsPoint(0, -1, -1, {0, 0, 0}));
        ao2 = finalAO(countNeighborsPoint(-1, -1, 1, {0, 0, 1}), countNeighborsPoint(-1, -1, 0, {0, 0, 1}),
                      countNeighborsPoint(0, -1, 1, {0, 0, 1}));
        ao3 = finalAO(countNeighborsPoint(1, -1, -1, {1, 0, 0}), countNeighborsPoint(1, -1, 0, {1, 0, 0}),
                      countNeighborsPoint(0, -1, -1, {1, 0, 0}));
        ao4 = finalAO(countNeighborsPoint(1, -1, 1, {1, 0, 1}), countNeighborsPoint(1, -1, 0, {1, 0, 1}),
                      countNeighborsPoint(0, -1, 1, {1, 0, 1}));
        break;
    }
    case OMVoxelFacing::PosY: {
        ao1 = finalAO(countNeighborsPoint(-1, 1, -1, {0, 1, 0}), countNeighborsPoint(-1, 1, 0, {0, 1, 0}),
                      countNeighborsPoint(0, 1, -1, {0, 1, 0}));
        ao2 = finalAO(countNeighborsPoint(-1, 1, 1, {0, 1, 1}), countNeighborsPoint(-1, 1, 0, {0, 1, 1}),
                      countNeighborsPoint(0, 1, 1, {0, 1, 1}));
        ao3 = finalAO(countNeighborsPoint(1, 1, -1, {1, 1, 0}), countNeighborsPoint(1, 1, 0, {1, 1, 0}),
                      countNeighborsPoint(0, 1, -1, {1, 1, 0}));
        ao4 = finalAO(countNeighborsPoint(1, 1, 1, {1, 1, 1}), countNeighborsPoint(1, 1, 0, {1, 1, 1}),
                      countNeighborsPoint(0, 1, 1, {1, 1, 1}));
        break;
    }
    case OMVoxelFacing::NegX: {
        ao1 = finalAO(countNeighborsPoint(-1, 1, -1, {0, 1, 0}), countNeighborsPoint(-1, 1, 0, {0, 1, 0}),
                      countNeighborsPoint(-1, 0, -1, {0, 1, 0}));
        ao2 = finalAO(countNeighborsPoint(-1, -1, -1, {0, 0, 0}), countNeighborsPoint(-1, -1, 0, {0, 0, 0}),
                      countNeighborsPoint(-1, 0, -1, {0, 0, 0}));
        ao3 = finalAO(countNeighborsPoint(-1, 1, 1, {0, 1, 1}), countNeighborsPoint(-1, 1, 0, {0, 1, 1}),
                      countNeighborsPoint(-1, 0, 1, {0, 1, 1}));
        ao4 = finalAO(countNeighborsPoint(-1, -1, 1, {0, 0, 1}), countNeighborsPoint(-1, -1, 0, {0, 0, 1}),
                      countNeighborsPoint(-1, 0, 1, {0, 0, 1}));
        break;
    }
    case OMVoxelFacing::PosX: {
        ao1 = finalAO(countNeighborsPoint(1, 1, 1, {1, 1, 1}), countNeighborsPoint(1, 1, 0, {1, 1, 1}),
                      countNeighborsPoint(1, 0, 1, {1, 1, 1}));
        ao2 = finalAO(countNeighborsPoint(1, -1, 1, {1, 0, 1}), countNeighborsPoint(1, -1, 0, {1, 0, 1}),
                      countNeighborsPoint(1, 0, 1, {1, 0, 1}));
        ao3 = finalAO(countNeighborsPoint(1, 1, -1, {1, 1, 0}), countNeighborsPoint(1, 1, 0, {1, 1, 0}),
                      countNeighborsPoint(1, 0, -1, {1, 1, 0}));
        ao4 = finalAO(countNeighborsPoint(1, -1, -1, {1, 0, 0}), countNeighborsPoint(1, -1, 0, {1, 0, 0}),
                      countNeighborsPoint(1, 0, -1, {1, 0, 0}));
        break;
    }
    case OMVoxelFacing::NegZ: {
        ao1 = finalAO(countNeighborsPoint(1, 1, -1, {1, 1, 0}), countNeighborsPoint(1, 0, -1, {1, 1, 0}),
                      countNeighborsPoint(0, 1, -1, {1, 1, 0}));
        ao2 = finalAO(countNeighborsPoint(1, -1, -1, {1, 0, 0}), countNeighborsPoint(1, 0, -1, {1, 0, 0}),
                      countNeighborsPoint(0, -1, -1, {1, 0, 0}));
        ao3 = finalAO(countNeighborsPoint(-1, 1, -1, {0, 1, 0}), countNeighborsPoint(-1, 0, -1, {0, 1, 0}),
                      countNeighborsPoint(0, 1, -1, {0, 1, 0}));
        ao4 = finalAO(countNeighborsPoint(-1, -1, -1, {0, 0, 0}), countNeighborsPoint(-1, 0, -1, {0, 0, 0}),
                      countNeighborsPoint(0, -1, -1, {0, 0, 0}));
        break;
    }
    case OMVoxelFacing::PosZ: {
        ao1 = finalAO(countNeighborsPoint(-1, 1, 1, {0, 1, 1}), countNeighborsPoint(-1, 0, 1, {0, 1, 1}),
                      countNeighborsPoint(0, 1, 1, {0, 1, 1}));
        ao2 = finalAO(countNeighborsPoint(-1, -1, 1, {0, 0, 1}), countNeighborsPoint(-1, 0, 1, {0, 0, 1}),
                      countNeighborsPoint(0, -1, 1, {0, 0, 1}));
        ao3 = finalAO(countNeighborsPoint(1, 1, 1, {1, 1, 1}), countNeighborsPoint(1, 0, 1, {1, 1, 1}),
                      countNeighborsPoint(0, 1, 1, {1, 1, 1}));
        ao4 = finalAO(countNeighborsPoint(1, -1, 1, {1, 0, 1}), countNeighborsPoint(1, 0, 1, {1, 0, 1}),
                      countNeighborsPoint(0, -1, 1, {1, 0, 1}));
        break;
    }
    default:
        break;
    }
    return {ao1, ao2, ao3, ao4};
}

auto OMVoxelCompiler::checkSkipRendering(
    const world::OMChunk<16> &chunk, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
    glm::ivec3 v, OMVoxelFacing f, uint32_t id) -> bool
{
    switch (f)
    {
    default:
    case None:
        return false;
    case NegX:
        v.x -= 1;
        break;
    case NegY:
        v.y -= 1;
        break;
    case NegZ:
        v.z -= 1;
        break;
    case PosX:
        v.x += 1;
        break;
    case PosY:
        v.y += 1;
        break;
    case PosZ:
        v.z += 1;
        break;
    }
    return handler->querySkipsRendering(id, queryBlockState(chunk, externalAccessor, v.x, v.y, v.z), f);
}

namespace
{
constexpr float faceEpsilon = 1e-4f;

struct FacePlane
{
    int axis;
    int u;
    int v;
    bool positive;
    float uMin;
    float uMax;
    float vMin;
    float vMax;
    float plane;
};

struct FaceRect
{
    float uMin;
    float uMax;
    float vMin;
    float vMax;
};

auto facePlane(const OMVoxelAABB &box, OMVoxelFacing face) -> FacePlane
{
    switch (face)
    {
    case NegX:
    case PosX:
        return {0,
                1,
                2,
                face == PosX,
                box.offset.y,
                box.offset.y + box.size.y,
                box.offset.z,
                box.offset.z + box.size.z,
                face == PosX ? box.offset.x + box.size.x : box.offset.x};
    case NegY:
    case PosY:
        return {1,
                0,
                2,
                face == PosY,
                box.offset.x,
                box.offset.x + box.size.x,
                box.offset.z,
                box.offset.z + box.size.z,
                face == PosY ? box.offset.y + box.size.y : box.offset.y};
    case NegZ:
    case PosZ:
        return {2,
                0,
                1,
                face == PosZ,
                box.offset.x,
                box.offset.x + box.size.x,
                box.offset.y,
                box.offset.y + box.size.y,
                face == PosZ ? box.offset.z + box.size.z : box.offset.z};
    default:
        return {0, 1, 2, false, 0, 0, 0, 0, 0};
    }
}

auto component(const glm::vec3 &v, int axis) -> float
{
    return axis == 0 ? v.x : (axis == 1 ? v.y : v.z);
}

auto rectCovers(const std::vector<FaceRect> &rects, const FaceRect &target) -> bool
{
    std::vector<float> us = {target.uMin, target.uMax};
    std::vector<float> vs = {target.vMin, target.vMax};

    for (const auto &r : rects)
    {
        us.push_back(r.uMin);
        us.push_back(r.uMax);
        vs.push_back(r.vMin);
        vs.push_back(r.vMax);
    }

    auto uniqueSorted = [](std::vector<float> &values) {
        std::sort(values.begin(), values.end());
        values.erase(
            std::unique(values.begin(), values.end(), [](float a, float b) { return std::abs(a - b) <= faceEpsilon; }),
            values.end());
    };
    uniqueSorted(us);
    uniqueSorted(vs);

    for (size_t ui = 0; ui + 1 < us.size(); ++ui)
    {
        const float u0 = us[ui];
        const float u1 = us[ui + 1];
        if (u1 - u0 <= faceEpsilon)
        {
            continue;
        }
        for (size_t vi = 0; vi + 1 < vs.size(); ++vi)
        {
            const float v0 = vs[vi];
            const float v1 = vs[vi + 1];
            if (v1 - v0 <= faceEpsilon)
            {
                continue;
            }

            const float uc = (u0 + u1) * 0.5f;
            const float vc = (v0 + v1) * 0.5f;
            bool covered = false;
            for (const auto &r : rects)
            {
                if (uc >= r.uMin - faceEpsilon && uc <= r.uMax + faceEpsilon && vc >= r.vMin - faceEpsilon &&
                    vc <= r.vMax + faceEpsilon)
                {
                    covered = true;
                    break;
                }
            }
            if (!covered)
            {
                return false;
            }
        }
    }
    return true;
}
} // namespace

auto OMVoxelCompiler::checkVoxelOcculusion(
    const world::OMChunk<16> &chunk, std::function<uint32_t(glm::ivec3, int64_t, int64_t, int64_t)> externalAccessor,
    glm::ivec3 v, OMVoxelFacing face, OMVoxelFacing cullFace, uint32_t id, int pid) -> bool
{
    if (face == None || cullFace == None || pid < 0)
    {
        return false;
    }

    const auto currentModel = modelFetcher(id, chunk.chunkx, chunk.chunky, chunk.chunkz, v.x, v.y, v.z);
    if (handler->queryNumParts(currentModel) <= pid)
    {
        return false;
    }

    const auto box = handler->queryPartAABB(currentModel, pid);
    const auto planeInfo = facePlane(box, face);
    const FaceRect faceRect{planeInfo.uMin, planeInfo.uMax, planeInfo.vMin, planeInfo.vMax};
    if (faceRect.uMax - faceRect.uMin <= faceEpsilon || faceRect.vMax - faceRect.vMin <= faceEpsilon)
    {
        return false;
    }

    auto rectsFromModel = [&](int modelId, int skipPid, float axisShift) {
        std::vector<FaceRect> rects;
        const int parts = handler->queryNumParts(modelId);
        for (int part = 0; part < parts; ++part)
        {
            if (part == skipPid)
            {
                continue;
            }

            auto partBox = handler->queryPartAABB(modelId, part);
            partBox.offset[planeInfo.axis] += axisShift;
            const float partMin = component(partBox.offset, planeInfo.axis);
            const float partMax = partMin + component(partBox.size, planeInfo.axis);

            // The occluder must start exactly at this face plane and extend to
            // the outside. A part behind the face must not cull it.
            if (planeInfo.positive)
            {
                if (partMin < planeInfo.plane - faceEpsilon || partMin > planeInfo.plane + faceEpsilon)
                {
                    continue;
                }
            }
            else
            {
                if (partMax > planeInfo.plane + faceEpsilon || partMax < planeInfo.plane - faceEpsilon)
                {
                    continue;
                }
            }

            const float u0 = std::max(faceRect.uMin, component(partBox.offset, planeInfo.u));
            const float u1 =
                std::min(faceRect.uMax, component(partBox.offset, planeInfo.u) + component(partBox.size, planeInfo.u));
            const float v0 = std::max(faceRect.vMin, component(partBox.offset, planeInfo.v));
            const float v1 =
                std::min(faceRect.vMax, component(partBox.offset, planeInfo.v) + component(partBox.size, planeInfo.v));
            if (u1 - u0 > faceEpsilon && v1 - v0 > faceEpsilon)
            {
                rects.push_back({u0, u1, v0, v1});
            }
        }
        return rects;
    };

    // Other parts of the same block can cover an internal face, without any
    // block-boundary lookup.
    auto rects = rectsFromModel(currentModel, pid, 0.0f);
    if (rectCovers(rects, faceRect))
    {
        return true;
    }

    const bool boundary = planeInfo.positive ? planeInfo.plane >= 16.0f - faceEpsilon : planeInfo.plane <= faceEpsilon;
    if (!boundary || face != cullFace)
    {
        return false;
    }

    glm::ivec3 n = v;
    float axisShift = 0.0f;
    switch (cullFace)
    {
    case NegX:
        n.x -= 1;
        axisShift = -16.0f;
        break;
    case NegY:
        n.y -= 1;
        axisShift = -16.0f;
        break;
    case NegZ:
        n.z -= 1;
        axisShift = -16.0f;
        break;
    case PosX:
        n.x += 1;
        axisShift = 16.0f;
        break;
    case PosY:
        n.y += 1;
        axisShift = 16.0f;
        break;
    case PosZ:
        n.z += 1;
        axisShift = 16.0f;
        break;
    default:
        return false;
    }

    const auto neighbourState = queryBlockState(chunk, externalAccessor, n.x, n.y, n.z);
    const auto neighbourModel = modelFetcher(neighbourState, chunk.chunkx, chunk.chunky, chunk.chunkz, n.x, n.y, n.z);

    if (handler->queryTranslucent(neighbourModel) || !handler->querySolidRender(neighbourModel))
    {
        return false;
    }

    if (handler->querySoild(neighbourModel))
    {
        return true;
    }

    rects = rectsFromModel(neighbourModel, -1, axisShift);
    return rectCovers(rects, faceRect);
}

} // namespace openminecraft::renderer::common::wrap::voxel