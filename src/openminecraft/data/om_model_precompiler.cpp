#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "openminecraft-shell/data/om_model_precompiler.hpp"
#include "fmt/format.h"
#include "glm/common.hpp"
#include "glm/fwd.hpp"
#include "openminecraft-shell/data/block/om_block_registry.hpp"
#include "openminecraft-shell/data/block/om_blockstate_registry.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/compiler/json/om_compiler_ast_builder_json.hpp"
#include "openminecraft/compiler/json/om_compiler_ast_json.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_objects.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

using namespace openminecraft;
using namespace openminecraft::compiler;
using namespace openminecraft::renderer::common::wrap::voxel;

namespace openminecraft::shell::data
{
OMModelPrecompiler::OMModelPrecompiler(std::string root, OMTextureAtlas *atlas)
    : root(std::move(root)), logger("OMModelPrecompiler", this), textureAtlas(*atlas)
{
}

auto OMModelPrecompiler::queryPartRotationAngleExt(int modelId, int pid) -> glm::vec3
{
    return blockModels[modelId].parts[pid].rotateAngleExt;
}

static auto fromSide(std::string s) -> OMModelCullSide
{
    using namespace binary::hash;
    switch (hash_compile_time(s.c_str()))
    {
    case "down"_hash:
        return Down;
    case "up"_hash:
        return Up;
    case "south"_hash:
        return South;
    case "north"_hash:
        return North;
    case "east"_hash:
        return East;
    case "west"_hash:
        return West;
    default:
        return None;
    }
}

static auto fromAxis(std::string s) -> OMModelAxis
{
    using namespace binary::hash;
    switch (hash_compile_time(s.c_str()))
    {
    default:
    case "x"_hash:
        return X;
    case "y"_hash:
        return Y;
    case "z"_hash:
        return Z;
    }
}
auto OMModelPrecompiler::querySkipsRendering(int modelId, int tgmodelId,
                                             openminecraft::renderer::common::wrap::voxel::OMVoxelFacing d) -> bool
{
    const auto &b = block::blockStateRegistry.idToRegistry[modelId];
    const auto &tb = block::blockStateRegistry.idToRegistry[tgmodelId];
    return block::blockRegistry.getRegistry(b.block)->skipsRendering(b, tb, d);
}
auto OMModelPrecompiler::queryTranslucent(int modelId) -> bool
{
    return blockModels[modelId].translucent;
}

auto OMModelPrecompiler::querySolidRender(int modelId) -> bool
{
    return blockModels[modelId].isSolidRender;
}

auto OMModelPrecompiler::queryPartFaceSecondaryTexture(int modelId, int pid,
                                                       openminecraft::renderer::common::wrap::voxel::OMVoxelFacing f)
    -> bool
{
    switch (f)
    {
    default:
    case ::NegX:
        return blockModels[modelId].parts[pid].west.secondaryTex;
    case ::PosX:
        return blockModels[modelId].parts[pid].east.secondaryTex;
    case ::NegZ:
        return blockModels[modelId].parts[pid].north.secondaryTex;
    case ::PosZ:
        return blockModels[modelId].parts[pid].south.secondaryTex;
    case ::NegY:
        return blockModels[modelId].parts[pid].down.secondaryTex;
    case ::PosY:
        return blockModels[modelId].parts[pid].up.secondaryTex;
    }
}
auto OMModelPrecompiler::queryPartRotationAngleF(int modelId, int pid) -> float
{
    return blockModels[modelId].parts[pid].rotateAngle;
}

auto OMModelPrecompiler::queryPartComplex(int modelId, int pid) -> bool
{
    return blockModels[modelId].partComplex[pid];
}

auto OMModelPrecompiler::queryPartShade(int modelId, int pid) -> bool
{
    return blockModels[modelId].parts[pid].shade;
}
auto OMModelPrecompiler::queryPartAmbientOcclusion(int modelId, int pid) -> bool
{
    return blockModels[modelId].partAmbientOcculusion[pid];
}

auto OMModelPrecompiler::queryOcclusionShape(int modelId) -> OMVoxelShape
{
    std::vector<OMVoxelAABB> r;
    for (auto &part : blockModels[modelId].parts)
    {
        r.emplace_back(OMVoxelAABB{part.from, part.to - part.from});
    }

    return {r};
}

auto OMModelPrecompiler::queryPartAABB(int modelId, int pid) -> OMVoxelAABB
{
    return {blockModels[modelId].parts[pid].from,
            blockModels[modelId].parts[pid].to - blockModels[modelId].parts[pid].from};
}

auto OMModelPrecompiler::queryNumParts(int modelId) -> int
{
    return blockModels[modelId].parts.size();
}
auto OMModelPrecompiler::queryPartFaceEnabled(int modelId, int pid, ::OMVoxelFacing f) -> bool
{
    switch (f)
    {
    default:
    case ::NegX:
        return blockModels[modelId].parts[pid].enableWest;
    case ::PosX:
        return blockModels[modelId].parts[pid].enableEast;
    case ::NegZ:
        return blockModels[modelId].parts[pid].enableNorth;
    case ::PosZ:
        return blockModels[modelId].parts[pid].enableSouth;
    case ::NegY:
        return blockModels[modelId].parts[pid].enableDown;
    case ::PosY:
        return blockModels[modelId].parts[pid].enableUp;
    }
}
auto OMModelPrecompiler::queryPartFaceTex(int modelId, int pid, ::OMVoxelFacing f) -> int
{
    switch (f)
    {
    default:
    case ::NegX:
        return blockModels[modelId].parts[pid].west.textureid;
    case ::PosX:
        return blockModels[modelId].parts[pid].east.textureid;
    case ::NegZ:
        return blockModels[modelId].parts[pid].north.textureid;
    case ::PosZ:
        return blockModels[modelId].parts[pid].south.textureid;
    case ::NegY:
        return blockModels[modelId].parts[pid].down.textureid;
    case ::PosY:
        return blockModels[modelId].parts[pid].up.textureid;
    }
}
auto cullFaceTo(OMModelCullSide s) -> ::OMVoxelFacing
{
    switch (s)
    {
    case Down:
        return NegY;
    case Up:
        return PosY;
    case South:
        return PosZ;
    case North:
        return NegZ;
    case West:
        return NegX;
    case East:
        return PosX;
    default:
    case None:
        return openminecraft::renderer::common::wrap::voxel::None;
    }
}
auto OMModelPrecompiler::queryPartFaceCull(int modelId, int pid, ::OMVoxelFacing f) -> ::OMVoxelFacing
{
    switch (f)
    {
    default:
    case ::NegX:
        return cullFaceTo(blockModels[modelId].parts[pid].west.cull);
    case ::PosX:
        return cullFaceTo(blockModels[modelId].parts[pid].east.cull);
    case ::NegZ:
        return cullFaceTo(blockModels[modelId].parts[pid].north.cull);
    case ::PosZ:
        return cullFaceTo(blockModels[modelId].parts[pid].south.cull);
    case ::NegY:
        return cullFaceTo(blockModels[modelId].parts[pid].down.cull);
    case ::PosY:
        return cullFaceTo(blockModels[modelId].parts[pid].up.cull);
    }
}

auto OMModelPrecompiler::queryPartFaceUV(int modelId, int pid,
                                         openminecraft::renderer::common::wrap::voxel::OMVoxelFacing f) -> glm::vec4
{
    glm::vec2 uv0, uv1;
    switch (f)
    {
    default:
    case ::NegX:
        uv0 = blockModels[modelId].parts[pid].west.uv0;
        uv1 = blockModels[modelId].parts[pid].west.uv1;
        break;
    case ::PosX:
        uv0 = blockModels[modelId].parts[pid].east.uv0;
        uv1 = blockModels[modelId].parts[pid].east.uv1;
        break;
    case ::NegZ:
        uv0 = blockModels[modelId].parts[pid].north.uv0;
        uv1 = blockModels[modelId].parts[pid].north.uv1;
        break;
    case ::PosZ:
        uv0 = blockModels[modelId].parts[pid].south.uv0;
        uv1 = blockModels[modelId].parts[pid].south.uv1;
        break;
    case ::NegY:
        uv0 = blockModels[modelId].parts[pid].down.uv0;
        uv1 = blockModels[modelId].parts[pid].down.uv1;
        break;
    case ::PosY:
        uv0 = blockModels[modelId].parts[pid].up.uv0;
        uv1 = blockModels[modelId].parts[pid].up.uv1;
        break;
    }
    return {uv0.x, uv0.y, uv1.x, uv1.y};
}

auto OMModelPrecompiler::queryPartFaceRotation(int modelId, int pid,
                                               openminecraft::renderer::common::wrap::voxel::OMVoxelFacing f) -> int
{
    switch (f)
    {
    default:
    case ::NegX:
        return blockModels[modelId].parts[pid].west.rotation / 90;
    case ::PosX:
        return blockModels[modelId].parts[pid].east.rotation / 90;
    case ::NegZ:
        return blockModels[modelId].parts[pid].north.rotation / 90;
    case ::PosZ:
        return blockModels[modelId].parts[pid].south.rotation / 90;
    case ::NegY:
        return blockModels[modelId].parts[pid].down.rotation / 90;
    case ::PosY:
        return blockModels[modelId].parts[pid].up.rotation / 90;
    }
}

auto OMModelPrecompiler::queryPartFaceTint(int modelId, int pid, ::OMVoxelFacing f) -> int
{
    switch (f)
    {
    default:
    case ::NegX:
        return blockModels[modelId].parts[pid].west.tintindex;
    case ::PosX:
        return blockModels[modelId].parts[pid].east.tintindex;
    case ::NegY:
        return blockModels[modelId].parts[pid].down.tintindex;
    case ::PosY:
        return blockModels[modelId].parts[pid].up.tintindex;
    case ::NegZ:
        return blockModels[modelId].parts[pid].north.tintindex;
    case ::PosZ:
        return blockModels[modelId].parts[pid].south.tintindex;
    }
}

auto OMModelPrecompiler::queryPartRotationAxis(int modelId, int pid) -> int
{
    return blockModels[modelId].parts[pid].rotateAxis;
}

auto OMModelPrecompiler::queryPartRotationCenter(int modelId, int pid) -> glm::vec3
{
    return blockModels[modelId].parts[pid].rotateOrigin;
}

auto OMModelPrecompiler::queryPartRotationAngle(int modelId, int pid) -> int
{
    return static_cast<float>(blockModels[modelId].parts[pid].rotateAngle + 45) / 22.5f;
}

auto OMModelPrecompiler::querySoild(int modelId) -> bool
{
    return blockModels[modelId].soild;
}

auto OMModelPrecompiler::queryFullBlock(int modelId) -> bool
{
    // Models such as stone/ore variants use a full cube plus an overlay part.
    // A single full-cube part is enough to make the block a full occluder.
    constexpr float eps = 0.001f;

    for (const auto &part : blockModels[modelId].parts)
    {
        if (part.from.x <= eps && part.from.y <= eps && part.from.z <= eps && part.to.x >= 16.0f - eps &&
            part.to.y >= 16.0f - eps && part.to.z >= 16.0f - eps)
        {
            return true;
        }
    }

    return false;
}

auto OMModelPrecompiler::queryFluid(int modelId) -> bool
{
    return blockModels[modelId].fluid;
}

auto OMModelPrecompiler::queryFluidLava(int modelId) -> bool
{
    return blockModels[modelId].fluidLava;
}

auto OMModelPrecompiler::queryFluidFalling(int modelId) -> bool
{
    return blockModels[modelId].fluidFalling;
}
auto OMModelPrecompiler::queryFluidLevel(int modelId) -> uint8_t
{
    return blockModels[modelId].fluidLevel;
}
auto OMModelPrecompiler::queryFluidTex(int modelId) -> int
{
    return blockModels[modelId].fluidTex;
}
auto OMModelPrecompiler::queryWaterlogged(int modelId) -> bool
{
    return blockModels[modelId].waterlogged;
}

auto OMModelPrecompiler::queryFluidSame(int modelId1, int modelId2) -> bool
{
    return fluidModels[modelId1] == fluidModels[modelId2];
}

auto OMModelPrecompiler::queryFluidTexSec(int modelId) -> int
{
    return blockModels[modelId].fluidTexFlow;
}

auto OMModelPrecompiler::loadModelPart(OMIdentifier i) -> int
{
    return loadModelPartWithArgs(i, 0, 0, 0, false);
}

// INFO: General rorate function
auto rotateVoxelElement90(const glm::vec3 &from, const glm::vec3 &to, int i) -> std::pair<glm::vec3, glm::vec3>
{
    glm::vec3 corners[8] = {{from.x, from.y, from.z}, {from.x, from.y, to.z}, {from.x, to.y, from.z},
                            {from.x, to.y, to.z},     {to.x, from.y, from.z}, {to.x, from.y, to.z},
                            {to.x, to.y, from.z},     {to.x, to.y, to.z}};

    auto newMin = glm::vec3(16, 16, 16);
    auto newMax = glm::vec3(0, 0, 0);

    for (const auto &p : corners)
    {
        auto centered = p - glm::vec3(8, 8, 8);

        glm::vec3 rotated;
        switch (i)
        {
        case 0:
            rotated = {centered.x, -centered.z, centered.y};
            break;
        case 1:
            rotated = {centered.z, centered.y, -centered.x};
            break;
        case 2:
            rotated = {-centered.y, centered.x, centered.z};
            break;
        }
        auto finalPos = rotated + glm::vec3(8, 8, 8);

        newMin = glm::min(newMin, finalPos);
        newMax = glm::max(newMax, finalPos);
    }

    return {newMin, newMax};
}

static auto rotatePoint90(const glm::vec3 &p, int mode) -> glm::vec3
{
    const auto c = p - glm::vec3(8, 8, 8);
    glm::vec3 r;
    switch (mode)
    {
    case 0:
        r = {c.x, -c.z, c.y};
        break;
    case 1:
        r = {c.z, c.y, -c.x};
        break;
    default:
        r = {-c.y, c.x, c.z};
        break;
    }
    return r + glm::vec3(8, 8, 8);
}

static auto rotateVector90(const glm::vec3 &v, int mode) -> glm::vec3
{
    switch (mode)
    {
    case 0:
        return {v.x, -v.z, v.y};
    case 1:
        return {v.z, v.y, -v.x};
    default:
        return {-v.y, v.x, v.z};
    }
}

// Blockstate x/y/z rotations must also rotate the element-local rotation
// frame. Leaving "axis": "y" untouched while rotating the AABB is what made
// some amethyst clusters (cross model, 45 degrees around Y) appear rotated
// into an unrelated direction.
static void rotateElementRotation(const std::shared_ptr<openminecraft::compiler::json::OMJsonNode> &element, int mode)
{
    auto &map = element->getMap();
    if (!map.count("rotation"))
    {
        return;
    }

    auto &rot = map["rotation"];
    auto &rmap = rot->getMap();
    if (rmap.count("origin"))
    {
        auto &origin = rmap["origin"]->getArray();
        auto p =
            glm::vec3(origin[0]->getNumberFloating(), origin[1]->getNumberFloating(), origin[2]->getNumberFloating());
        p = rotatePoint90(p, mode);
        origin[0] = std::make_shared<openminecraft::compiler::json::OMJsonNodeNumber>(p.x);
        origin[1] = std::make_shared<openminecraft::compiler::json::OMJsonNodeNumber>(p.y);
        origin[2] = std::make_shared<openminecraft::compiler::json::OMJsonNodeNumber>(p.z);
    }

    if (!rmap.count("axis"))
    {
        return;
    }

    const auto axis = rmap["axis"]->getString();
    glm::vec3 axisVector = axis == "x" ? glm::vec3(1, 0, 0) : (axis == "y" ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1));
    const auto rotatedAxis = rotateVector90(axisVector, mode);

    std::string newAxis;
    float sign = 1.0f;
    if (std::abs(rotatedAxis.x) > 0.5f)
    {
        newAxis = "x";
        sign = rotatedAxis.x < 0 ? -1.0f : 1.0f;
    }
    else if (std::abs(rotatedAxis.y) > 0.5f)
    {
        newAxis = "y";
        sign = rotatedAxis.y < 0 ? -1.0f : 1.0f;
    }
    else
    {
        newAxis = "z";
        sign = rotatedAxis.z < 0 ? -1.0f : 1.0f;
    }

    rmap["axis"] = std::make_shared<openminecraft::compiler::json::OMJsonNodeString>(newAxis);
    if (sign < 0 && rmap.count("angle"))
    {
        rmap["angle"] =
            std::make_shared<openminecraft::compiler::json::OMJsonNodeNumber>(-rmap["angle"]->getNumberFloating());
    }
}

auto OMModelPrecompiler::loadModelPartWithArgs(OMIdentifier i, int xrot, int yrot, int zrot, bool lockuv) -> int
{
    logger.info("precompiling {}:{} @ {}, rot {} {} {}, uvlock {}", i.namesp, i.path, modelId, xrot, yrot, zrot,
                lockuv ? "true" : "false");
    modelParts.resize(modelId + 1);
    modelAmbientOcculusion.resize(modelId + 1);
    modelComplex.resize(modelId + 1);

    modelComplex[modelId] = false;
    modelParts[modelId] = {};

    modelPartIdents[i] = modelId;

    auto prem = precompile(i);
    modelAmbientOcculusion[modelId] =
        prem->getMap().count("ambientocclusion") ? prem->getMap()["ambientocclusion"]->getBoolean() : true;
    auto pre = prem->getMap()["elements"];

    if (pre)
    {
        for (auto &p : pre->getArray())
        {
            auto &faces = p->getMap()["faces"]->getMap();
            auto fetchface = [&](std::string f) -> std::shared_ptr<openminecraft::compiler::json::OMJsonNode> {
                auto face = faces.find(f);
                if (face != faces.end())
                {
                    return face->second;
                }
                else
                {
                    return nullptr;
                }
            };
            auto putface = [&](std::string f, std::shared_ptr<openminecraft::compiler::json::OMJsonNode> a) {
                if (!a)
                {
                    if (faces.count(f))
                    {
                        faces.erase(f);
                    }
                }
                else
                {
                    if (a->getMap().count("cullface"))
                    {
                        a->getMap()["cullface"] = std::make_shared<json::OMJsonNodeString>(f);
                    }
                    faces[f] = a;
                }
            };
            auto addFaceRotation = [&](const char *faceName, int amount) {
                auto face = fetchface(faceName);
                if (!face)
                {
                    return;
                }
                int rotation =
                    face->getMap().count("rotation") ? static_cast<int>(face->getMap()["rotation"]->getNumber()) : 0;
                rotation = (rotation + amount) % 360;
                if (rotation < 0)
                {
                    rotation += 360;
                }
                face->getMap()["rotation"] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(rotation));
            };
            struct FaceRotationEntry
            {
                const char *face;
                int degrees;
            };
            struct FaceMove
            {
                const char *from;
                const char *to;
            };

            constexpr std::array<const char *, 6> faceNames = {"north", "south", "east", "west", "up", "down"};
            enum FaceIndex : size_t
            {
                North,
                South,
                East,
                West,
                Up,
                Down
            };

            auto applyQuarterTurn = [&](int axis) {
                rotateElementRotation(p, axis);

                auto &from = p->getMap()["from"]->getArray();
                auto &to = p->getMap()["to"]->getArray();
                auto fromv =
                    glm::vec3{from[0]->getNumberFloating(), from[1]->getNumberFloating(), from[2]->getNumberFloating()};
                auto tov =
                    glm::vec3{to[0]->getNumberFloating(), to[1]->getNumberFloating(), to[2]->getNumberFloating()};
                auto rotated = rotateVoxelElement90(fromv, tov, axis);

                from[0] = std::make_shared<json::OMJsonNodeNumber>(rotated.first.x);
                from[1] = std::make_shared<json::OMJsonNodeNumber>(rotated.first.y);
                from[2] = std::make_shared<json::OMJsonNodeNumber>(rotated.first.z);
                to[0] = std::make_shared<json::OMJsonNodeNumber>(rotated.second.x);
                to[1] = std::make_shared<json::OMJsonNodeNumber>(rotated.second.y);
                to[2] = std::make_shared<json::OMJsonNodeNumber>(rotated.second.z);

                // UV deltas are applied to the old face before it is renamed.
                std::array<FaceRotationEntry, 6> faceRotations = {};
                std::array<FaceMove, 4> faceMoves = {};

                if (axis == 0)
                {
                    // The south face becomes down. North/south variants need
                    // the pre-rotation; east/west get it from the Y table.
                    const int southRotation = ((yrot % 180) + 180) % 180 == 0 ? 180 : 0;
                    faceRotations = {{{"north", 180},
                                      {"south", southRotation},
                                      {"east", 270},
                                      {"west", 90},
                                      {"up", 0},
                                      {"down", 0}}};
                    faceMoves = {{{"up", "south"}, {"south", "down"}, {"down", "north"}, {"north", "up"}}};
                }
                else if (axis == 1)
                {
                    faceRotations = {{{"north", 0},
                                      {"south", 0},
                                      {"east", 0},
                                      {"west", 0},
                                      {"up", 270},
                                      {"down", 90}}};
                    faceMoves = {{{"east", "north"}, {"north", "west"}, {"west", "south"}, {"south", "east"}}};
                }
                else
                {
                    faceRotations = {{{"north", 90},
                                      {"south", 270},
                                      {"east", 270},
                                      {"west", 90},
                                      {"up", 270},
                                      {"down", 90}}};
                    faceMoves = {{{"up", "west"}, {"west", "down"}, {"down", "east"}, {"east", "up"}}};
                }

                if (!lockuv)
                {
                    for (const auto &entry : faceRotations)
                    {
                        addFaceRotation(entry.face, entry.degrees);
                    }
                }

                // Fetch all six first, otherwise one rename can overwrite a
                // face that is still needed by the next rename.
                std::array<std::shared_ptr<openminecraft::compiler::json::OMJsonNode>, 6> original = {};
                for (size_t n = 0; n < faceNames.size(); ++n)
                {
                    original[n] = fetchface(faceNames[n]);
                }

                auto indexOf = [&](const char *name) -> size_t {
                    for (size_t n = 0; n < faceNames.size(); ++n)
                    {
                        if (std::strcmp(faceNames[n], name) == 0)
                        {
                            return n;
                        }
                    }
                    return 0;
                };

                for (const auto &move : faceMoves)
                {
                    putface(move.to, original[indexOf(move.from)]);
                }
            };

            auto applyFinalCorrections = [&]() {
                if (lockuv)
                {
                    return;
                }

                const int nx = ((xrot % 360) + 360) % 360;
                const int ny = ((yrot % 360) + 360) % 360;

                // Residual texture-basis corrections after the three quarter
                // turns. They are keyed by the original blockstate variant.
                //
                // north/south walls (x=90, y=0/180): the final east face comes
                // from the old east/west face, so it needs 180; the whole cross
                // also needs a half turn because north is the reference wall.
                //
                // east/west walls (x=90, y=90/270): the final south face comes
                // from the old east/west face and needs 180.
                struct VariantCorrection
                {
                    int x;
                    int y;
                    std::array<int, 6> faces;
                };
                static constexpr std::array<VariantCorrection, 4> corrections = {{
                    {90, 0, {{180, 180, 0, 180, 180, 180}}},
                    {90, 90, {{0, 180, 0, 0, 0, 0}}},
                    {90, 180, {{180, 180, 0, 180, 180, 180}}},
                    {90, 270, {{0, 180, 0, 0, 0, 0}}},
                }};

                for (const auto &correction : corrections)
                {
                    if (nx == correction.x && ny == correction.y)
                    {
                        for (size_t n = 0; n < faceNames.size(); ++n)
                        {
                            addFaceRotation(faceNames[n], correction.faces[n]);
                        }
                        return;
                    }
                }

                // A pure Y half-turn affects every face regardless of X.
                if (ny == 180)
                {
                    for (const char *face : faceNames)
                    {
                        addFaceRotation(face, 180);
                    }
                }
            };

            int txrot = xrot;
            int tyrot = yrot;
            int tzrot = zrot;
            while (txrot > 0)
            {
                applyQuarterTurn(0);
                txrot -= 90;
            }
            while (tyrot > 0)
            {
                applyQuarterTurn(1);
                tyrot -= 90;
            }
            while (tzrot > 0)
            {
                applyQuarterTurn(2);
                tzrot -= 90;
            }

            applyFinalCorrections();

            modelParts[modelId].emplace_back(wrapPart(p));
        }
    }

    modelId++;
    return modelId - 1;
}

static auto isNotInteger(std::shared_ptr<json::OMJsonNode> n) -> bool
{
    if (n == nullptr)
    {
        return true;
    }
    return std::abs(n->getNumberFloating() - std::round(n->getNumberFloating())) > 1e-5 || n->getNumberFloating() < 0;
}

auto OMModelPrecompiler::precompile(OMIdentifier name, bool subsitute)
    -> std::shared_ptr<openminecraft::compiler::json::OMJsonNode>
{
    auto modelStream = vfs::fsfetch(fmt::format("{}/{}/models/{}.json", root, name.namesp, name.path));
    json::OMJsonAstBuilder bld(std::make_shared<json::OMJsonTokenIter>(modelStream));
    auto document = bld.build();

    if (document->getMap().count("parent"))
    {
        auto sub = precompile(OMIdentifier(document->getMap()["parent"]->getString()), false);
        document->getMap().erase("parent");
        document->merge(sub);
    }

    if (subsitute)
    {
        auto &tex = document->getMap()["textures"]->getMap();

        for (auto &texture : tex)
        {
            if (texture.second->type() == openminecraft::compiler::json::Object)
            {
                texture.second = texture.second->getMap()["sprite"];
            }
        }

        for (auto &texture : tex)
        {
            if (texture.second->getString()[0] == '#' && tex.count(texture.second->getString().substr(1)))
            {
                texture.second = tex[texture.second->getString().substr(1)];
            }
        }

        if (document->getMap().count("elements"))
        {
            for (auto &elem : document->getMap()["elements"]->getArray())
            {
                auto fromn = elem->getMap()["from"]->getArray();
                auto ton = elem->getMap()["to"]->getArray();

                if (isNotInteger(fromn[0]) || isNotInteger(fromn[1]) || isNotInteger(fromn[2]) ||
                    isNotInteger(ton[0]) || isNotInteger(ton[1]) || isNotInteger(ton[2]))
                {
                    modelComplex[modelId] = true;
                }

                if (elem->getMap().count("rotation"))
                {
                    auto origin = elem->getMap()["rotation"]->getMap()["origin"]->getArray();
                    auto angle = elem->getMap()["rotation"]->getMap()["angle"];

                    if (isNotInteger(angle) || isNotInteger(origin[0]) || isNotInteger(origin[1]) ||
                        isNotInteger(origin[2]))
                    {
                        modelComplex[modelId] = true;
                    }
                }

                for (auto &fce : elem->getMap()["faces"]->getMap())
                {
                    auto text = fce.second->getMap()["texture"]->getString();

                    if (text[0] == '#' && tex.count(text.substr(1)))
                    {
                        fce.second->getMap()["texture"] = tex[text.substr(1)];
                    }

                    auto tident = OMIdentifier(fce.second->getMap()["texture"]->getString());
                    textureAtlas.addTexture(tident);
                    auto tsize = textureAtlas.textureSize(tident);
                    if (tsize.x > 16 || tsize.y > 16)
                    {
                        modelComplex[modelId] = true;
                        continue;
                    }

                    if (fce.second->getMap().count("uv"))
                    {
                        auto uv = fce.second->getMap()["uv"]->getArray();

                        if (isNotInteger(uv[0]) || isNotInteger(uv[1]) || isNotInteger(uv[2]) || isNotInteger(uv[3]))
                        {
                            modelComplex[modelId] = true;
                        }
                    }
                }
            }
        }
    }

    return document;
}

auto OMModelPrecompiler::wrapPart(std::shared_ptr<openminecraft::compiler::json::OMJsonNode> part) -> OMModelPart
{
    auto faces = part->getMap()["faces"]->getMap();
    auto from = part->getMap()["from"]->getArray();
    auto to = part->getMap()["to"]->getArray();
    auto ro = part->getMap().count("rotation") ? part->getMap()["rotation"] : nullptr;
    auto fromv = glm::vec3{from[0]->getNumberFloating(), from[1]->getNumberFloating(), from[2]->getNumberFloating()};
    auto tov = glm::vec3{to[0]->getNumberFloating(), to[1]->getNumberFloating(), to[2]->getNumberFloating()};
    auto roCenter = ro ? glm::vec3{static_cast<double>(ro->getMap()["origin"]->getArray()[0]->getNumberFloating()),
                                   static_cast<double>(ro->getMap()["origin"]->getArray()[1]->getNumberFloating()),
                                   static_cast<double>(ro->getMap()["origin"]->getArray()[2]->getNumberFloating())}
                       : glm::vec3{};
    auto roAngleExt =
        glm::vec3(ro && ro->getMap().count("x") ? static_cast<float>(ro->getMap()["x"]->getNumberFloating()) : 0.0f,
                  ro && ro->getMap().count("y") ? static_cast<float>(ro->getMap()["y"]->getNumberFloating()) : 0.0f,
                  ro && ro->getMap().count("z") ? static_cast<float>(ro->getMap()["z"]->getNumberFloating()) : 0.0f);

    return {faces.count("east") ? wrapFace(faces["east"], OMModelCullSide::East, fromv, tov) : OMModelFace(),
            faces.count("east") > 0,
            faces.count("west") ? wrapFace(faces["west"], OMModelCullSide::West, fromv, tov) : OMModelFace(),
            faces.count("west") > 0,
            faces.count("down") ? wrapFace(faces["down"], OMModelCullSide::Down, fromv, tov) : OMModelFace(),
            faces.count("down") > 0,
            faces.count("up") ? wrapFace(faces["up"], OMModelCullSide::Up, fromv, tov) : OMModelFace(),
            faces.count("up") > 0,
            faces.count("south") ? wrapFace(faces["south"], OMModelCullSide::South, fromv, tov) : OMModelFace(),
            faces.count("south") > 0,
            faces.count("north") ? wrapFace(faces["north"], OMModelCullSide::North, fromv, tov) : OMModelFace(),
            faces.count("north") > 0,
            fromv,
            tov,
            part->getMap().count("shade") ? part->getMap()["shade"]->getBoolean() : true,
            ro != nullptr,
            roCenter,
            ro && ro->getMap()["axis"] != nullptr ? fromAxis(ro->getMap()["axis"]->getString()) : (ro ? Any : X),
            ro && ro->getMap()["angle"] != nullptr ? ro->getMap()["angle"]->getNumberFloating() : 0.0,
            roAngleExt};
}

auto OMModelPrecompiler::wrapFace(std::shared_ptr<openminecraft::compiler::json::OMJsonNode> face, OMModelCullSide c,
                                  glm::vec3 from, glm::vec3 to) -> OMModelFace
{
    auto uv = face->getMap().count("uv") ? face->getMap()["uv"] : nullptr;
    glm::vec2 uv0, uv1;
    if (uv)
    {
        uv0 = {std::round(uv->getArray()[0]->getNumberFloating()), std::round(uv->getArray()[1]->getNumberFloating())};
        uv1 = {std::round(uv->getArray()[2]->getNumberFloating()), std::round(uv->getArray()[3]->getNumberFloating())};
    }
    else
    {
        uv0 = {0.0, 0.0};
        uv1 = {0.0, 0.0};
        switch (c)
        {
        case Up:
        case Down:
            uv0 = {from.x, from.z};
            uv1 = {to.x, to.z};
            break;
        case North:
        case South:
            uv0 = {from.x, from.y};
            uv1 = {to.x, to.y};
            break;
        case West:
        case East:
            uv0 = {from.z, from.y};
            uv1 = {to.z, to.y};
            break;
        case None:
        default:
            break;
        }
    }

    auto ident = OMIdentifier(face->getMap()["texture"]->getString());

    return {fromSide(face->getMap().count("cullface") ? face->getMap()["cullface"]->getString() : "none"),
            textureAtlas.subtex.count(ident) == 0 ? textureAtlas.addWideTexture(ident) : textureAtlas.subtex[ident],
            uv0,
            uv1,
            face->getMap().count("rotation") ? static_cast<int>(face->getMap()["rotation"]->getNumber()) : 0,
            textureAtlas.subtex.count(ident) == 0,
            face->getMap().count("tintindex") ? static_cast<int>(face->getMap()["tintindex"]->getNumber()) : -1};
}

auto OMModelPrecompiler::composeModel(std::vector<int> partids, bool soild, bool translucent, bool waterlogged,
                                      bool isSolidRender) -> int
{
    auto &m = blockModels.emplace_back();
    m.soild = soild;
    m.translucent = translucent;
    m.isSolidRender = isSolidRender;
    m.fluid = waterlogged;
    m.fluidFalling = false;
    m.fluidLevel = 0;
    m.fluidTex = textureAtlas.addTexture(OMIdentifier("minecraft:block/water_still"));
    m.waterlogged = waterlogged;
    for (auto i : partids)
    {
        for (auto &mp : modelParts[i])
        {
            m.parts.push_back(mp);
            m.partComplex.push_back(modelComplex[i]);
            m.partAmbientOcculusion.push_back(modelAmbientOcculusion[i]);
        }
    }

    fluidModels[blockModels.size() - 1] = OMIdentifier("minecraft:water");
    return blockModels.size() - 1;
}

auto OMModelPrecompiler::composeFluidModel(OMIdentifier ident, block::OMBlockFluidType fluidType, bool translucent,
                                           bool falling, uint8_t level) -> int
{
    // Both the texture and the "same fluid" grouping are determined by fluidType:
    // Waterlogged blocks (e.g. bubble_column) tagged FluidWater reuse water's texture and join water's group.
    const std::string fluidName = fluidType == block::OMBlockFluidType::FluidLava ? "lava" : "water";

    auto &m = blockModels.emplace_back();
    m.soild = false;
    m.translucent = translucent;
    m.fluid = true;
    m.fluidLava = fluidType == block::OMBlockFluidType::FluidLava;
    m.fluidFalling = falling;
    m.fluidLevel = level;
    m.fluidTex = textureAtlas.addTexture(OMIdentifier("minecraft:block/" + fluidName + "_still"));
    m.fluidTexFlow = textureAtlas.addWideTexture(OMIdentifier("minecraft:block/" + fluidName + "_flow"));
    m.waterlogged = false;

    fluidModels[blockModels.size() - 1] = OMIdentifier("minecraft:" + fluidName);

    return blockModels.size() - 1;
}
} // namespace openminecraft::shell::data
