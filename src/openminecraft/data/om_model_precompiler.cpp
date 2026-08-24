#include <memory>
#include <utility>
#include <vector>

#include "openminecraft-shell/data/om_model_precompiler.hpp"
#include "fmt/format.h"
#include "glm/common.hpp"
#include "glm/fwd.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

using namespace openminecraft;
using namespace openminecraft::io;
using namespace openminecraft::renderer::common::wrap;

namespace openminecraftshell::data
{
OMModelPrecompiler::OMModelPrecompiler(std::string root, OMTextureAtlas *atlas)
    : root(std::move(root)), logger("OMModelPrecompiler", this), textureAtlas(*atlas)
{
}

auto OMModelPrecompiler::queryPartRotationAngleExt(int bsid, int pid) -> glm::vec3
{
    return blockModels[bsid].parts[pid].rotateAngleExt;
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

auto OMModelPrecompiler::queryPartFaceSecondaryTexture(int bsid, int pid,
                                                       openminecraft::renderer::common::wrap::OMVoxelFacing f) -> bool
{
    switch (f)
    {
    default:
    case ::NegX:
        return blockModels[bsid].parts[pid].west.secondaryTex;
    case ::PosX:
        return blockModels[bsid].parts[pid].east.secondaryTex;
    case ::NegZ:
        return blockModels[bsid].parts[pid].north.secondaryTex;
    case ::PosZ:
        return blockModels[bsid].parts[pid].south.secondaryTex;
    case ::NegY:
        return blockModels[bsid].parts[pid].down.secondaryTex;
    case ::PosY:
        return blockModels[bsid].parts[pid].up.secondaryTex;
    }
}
auto OMModelPrecompiler::queryPartRotationAngleF(int bsid, int pid) -> float
{
    return blockModels[bsid].parts[pid].rotateAngle;
}

auto OMModelPrecompiler::queryPartComplex(int bsid, int pid) -> bool
{
    return blockModels[bsid].partComplex[pid];
}

auto OMModelPrecompiler::queryPartShade(int bsid, int pid) -> bool
{
    return blockModels[bsid].parts[pid].shade;
}
auto OMModelPrecompiler::queryPartAmbientOcclusion(int bsid, int pid) -> bool
{
    return blockModels[bsid].partAmbientOcculusion[pid];
}

auto OMModelPrecompiler::queryOcclusionShape(int bsid) -> OMVoxelShape
{
    std::vector<OMVoxelAABB> r;
    for (auto &pp : blockModels[bsid].parts)
    {
        r.emplace_back(OMVoxelAABB{pp.from, pp.to - pp.from});
    }

    return {r};
}

auto OMModelPrecompiler::queryPartAABB(int bsid, int pid) -> OMVoxelAABB
{
    return {blockModels[bsid].parts[pid].from, blockModels[bsid].parts[pid].to - blockModels[bsid].parts[pid].from};
}

auto OMModelPrecompiler::queryNumParts(int bsid) -> int
{
    return blockModels[bsid].parts.size();
}
auto OMModelPrecompiler::queryPartFaceEnabled(int bsid, int pid, ::OMVoxelFacing f) -> bool
{
    switch (f)
    {
    default:
    case ::NegX:
        return blockModels[bsid].parts[pid].enableWest;
    case ::PosX:
        return blockModels[bsid].parts[pid].enableEast;
    case ::NegZ:
        return blockModels[bsid].parts[pid].enableNorth;
    case ::PosZ:
        return blockModels[bsid].parts[pid].enableSouth;
    case ::NegY:
        return blockModels[bsid].parts[pid].enableDown;
    case ::PosY:
        return blockModels[bsid].parts[pid].enableUp;
    }
}
auto OMModelPrecompiler::queryPartFaceTex(int bsid, int pid, ::OMVoxelFacing f) -> int
{
    switch (f)
    {
    default:
    case ::NegX:
        return blockModels[bsid].parts[pid].west.textureid;
    case ::PosX:
        return blockModels[bsid].parts[pid].east.textureid;
    case ::NegZ:
        return blockModels[bsid].parts[pid].north.textureid;
    case ::PosZ:
        return blockModels[bsid].parts[pid].south.textureid;
    case ::NegY:
        return blockModels[bsid].parts[pid].down.textureid;
    case ::PosY:
        return blockModels[bsid].parts[pid].up.textureid;
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
    case None:
        return openminecraft::renderer::common::wrap::None;
    }
}
auto OMModelPrecompiler::queryPartFaceCull(int bsid, int pid, ::OMVoxelFacing f) -> ::OMVoxelFacing
{
    switch (f)
    {
    default:
    case ::NegX:
        return cullFaceTo(blockModels[bsid].parts[pid].west.cull);
    case ::PosX:
        return cullFaceTo(blockModels[bsid].parts[pid].east.cull);
    case ::NegZ:
        return cullFaceTo(blockModels[bsid].parts[pid].north.cull);
    case ::PosZ:
        return cullFaceTo(blockModels[bsid].parts[pid].south.cull);
    case ::NegY:
        return cullFaceTo(blockModels[bsid].parts[pid].down.cull);
    case ::PosY:
        return cullFaceTo(blockModels[bsid].parts[pid].up.cull);
    }
}

auto OMModelPrecompiler::queryPartFaceUV(int bsid, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing f)
    -> glm::vec4
{
    glm::vec2 uv0, uv1;
    switch (f)
    {
    default:
    case ::NegX:
        uv0 = blockModels[bsid].parts[pid].west.uv0;
        uv1 = blockModels[bsid].parts[pid].west.uv1;
        break;
    case ::PosX:
        uv0 = blockModels[bsid].parts[pid].east.uv0;
        uv1 = blockModels[bsid].parts[pid].east.uv1;
        break;
    case ::NegZ:
        uv0 = blockModels[bsid].parts[pid].north.uv0;
        uv1 = blockModels[bsid].parts[pid].north.uv1;
        break;
    case ::PosZ:
        uv0 = blockModels[bsid].parts[pid].south.uv0;
        uv1 = blockModels[bsid].parts[pid].south.uv1;
        break;
    case ::NegY:
        uv0 = blockModels[bsid].parts[pid].down.uv0;
        uv1 = blockModels[bsid].parts[pid].down.uv1;
        break;
    case ::PosY:
        uv0 = blockModels[bsid].parts[pid].up.uv0;
        uv1 = blockModels[bsid].parts[pid].up.uv1;
        break;
    }
    return {uv0.x, uv0.y, uv1.x, uv1.y};
}

auto OMModelPrecompiler::queryPartFaceRotation(int bsid, int pid,
                                               openminecraft::renderer::common::wrap::OMVoxelFacing f) -> int
{
    switch (f)
    {
    default:
    case ::NegX:
        return blockModels[bsid].parts[pid].west.rotation / 90;
    case ::PosX:
        return blockModels[bsid].parts[pid].east.rotation / 90;
    case ::NegZ:
        return blockModels[bsid].parts[pid].north.rotation / 90;
    case ::PosZ:
        return blockModels[bsid].parts[pid].south.rotation / 90;
    case ::NegY:
        return blockModels[bsid].parts[pid].down.rotation / 90;
    case ::PosY:
        return blockModels[bsid].parts[pid].up.rotation / 90;
    }
}

auto OMModelPrecompiler::queryPartRotationAxis(int bsid, int pid) -> int
{
    return blockModels[bsid].parts[pid].rotateAxis;
}

auto OMModelPrecompiler::queryPartRotationCenter(int bsid, int pid) -> glm::vec3
{
    return blockModels[bsid].parts[pid].rotateOrigin;
}

auto OMModelPrecompiler::queryPartRotationAngle(int bsid, int pid) -> int
{
    return static_cast<float>(blockModels[bsid].parts[pid].rotateAngle + 45) / 22.5f;
}

auto OMModelPrecompiler::querySoild(int bsid) -> bool
{
    return blockModels[bsid].soild;
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
            rotated = {-centered.z, centered.y, centered.x};
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
            auto &mm = p->getMap()["faces"]->getMap();
            auto fetchface = [&](std::string f) -> std::shared_ptr<openminecraft::io::json::OMJsonNode> {
                auto face = mm.find(f);
                if (face != mm.end())
                {
                    return face->second;
                }
                else
                {
                    return nullptr;
                }
            };
            auto putface = [&](std::string f, std::shared_ptr<openminecraft::io::json::OMJsonNode> a) {
                if (!a)
                {
                    if (mm.count(f))
                    {
                        mm.erase(f);
                    }
                }
                else
                {
                    if (a->getMap().count("cullface"))
                    {
                        a->getMap()["cullface"] = std::make_shared<json::OMJsonNodeString>(f);
                    }
                    mm[f] = a;
                }
            };
            auto xrot90 = [&]() {
                auto &from = p->getMap()["from"]->getArray();
                auto &to = p->getMap()["to"]->getArray();
                auto fromv =
                    glm::vec3{from[0]->getNumberFloating(), from[1]->getNumberFloating(), from[2]->getNumberFloating()};
                auto tov =
                    glm::vec3{to[0]->getNumberFloating(), to[1]->getNumberFloating(), to[2]->getNumberFloating()};
                auto vv = rotateVoxelElement90(fromv, tov, 0);

                from[0] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.first.x));
                from[1] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.first.y));
                from[2] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.first.z));
                to[0] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.second.x));
                to[1] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.second.y));
                to[2] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.second.z));

                auto fup = fetchface("up");
                auto fsouth = fetchface("south");
                auto fdown = fetchface("down");
                auto fnorth = fetchface("north");

                putface("south", fup);
                putface("down", fsouth);
                putface("north", fdown);
                putface("up", fnorth);

                auto fwest = fetchface("west");
                auto feast = fetchface("east");

                if (fwest)
                {
                    fwest->getMap()["rotation"] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(
                        ((fwest->getMap().count("rotation") ? fwest->getMap()["rotation"]->getNumber() : 0) +
                         (lockuv ? -90 : 90)) %
                        360));
                }
                if (feast)
                {
                    feast->getMap()["rotation"] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(
                        ((feast->getMap().count("rotation") ? feast->getMap()["rotation"]->getNumber() : 0) +
                         (lockuv ? -90 : 90)) %
                        360));
                }
            };
            auto yrot90 = [&]() {
                auto &from = p->getMap()["from"]->getArray();
                auto &to = p->getMap()["to"]->getArray();
                auto fromv =
                    glm::vec3{from[0]->getNumberFloating(), from[1]->getNumberFloating(), from[2]->getNumberFloating()};
                auto tov =
                    glm::vec3{to[0]->getNumberFloating(), to[1]->getNumberFloating(), to[2]->getNumberFloating()};
                auto vv = rotateVoxelElement90(fromv, tov, 1);

                from[0] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.first.x));
                from[1] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.first.y));
                from[2] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.first.z));
                to[0] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.second.x));
                to[1] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.second.y));
                to[2] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.second.z));

                auto feast = fetchface("east");
                auto fsouth = fetchface("south");
                auto fwest = fetchface("west");
                auto fnorth = fetchface("north");

                putface("south", feast);
                putface("west", fsouth);
                putface("north", fwest);
                putface("east", fnorth);

                auto fup = fetchface("up");
                auto fdown = fetchface("down");

                if (fup)
                {
                    fup->getMap()["rotation"] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(
                        ((fup->getMap().count("rotation") ? fup->getMap()["rotation"]->getNumber() : 0) +
                         (lockuv ? -90 : 90)) %
                        360));
                }
                if (fdown)
                {
                    fdown->getMap()["rotation"] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(
                        ((fdown->getMap().count("rotation") ? fdown->getMap()["rotation"]->getNumber() : 0) +
                         (lockuv ? -90 : 90)) %
                        360));
                }
            };
            auto zrot90 = [&]() {
                auto &from = p->getMap()["from"]->getArray();
                auto &to = p->getMap()["to"]->getArray();
                auto fromv =
                    glm::vec3{from[0]->getNumberFloating(), from[1]->getNumberFloating(), from[2]->getNumberFloating()};
                auto tov =
                    glm::vec3{to[0]->getNumberFloating(), to[1]->getNumberFloating(), to[2]->getNumberFloating()};
                auto vv = rotateVoxelElement90(fromv, tov, 2);

                from[0] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.first.x));
                from[1] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.first.y));
                from[2] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.first.z));
                to[0] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.second.x));
                to[1] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.second.y));
                to[2] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(vv.second.z));

                auto fup = fetchface("up");
                auto fwest = fetchface("west");
                auto fdown = fetchface("down");
                auto feast = fetchface("east");

                putface("west", fup);
                putface("down", fwest);
                putface("east", fdown);
                putface("up", feast);

                auto fnorth = fetchface("north");
                auto fsouth = fetchface("south");

                if (fnorth)
                {
                    fnorth->getMap()["rotation"] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(
                        ((fnorth->getMap().count("rotation") ? fnorth->getMap()["rotation"]->getNumber() : 0) +
                         (lockuv ? -90 : 90)) %
                        360));
                }
                if (fsouth)
                {
                    fsouth->getMap()["rotation"] = std::make_shared<json::OMJsonNodeNumber>(static_cast<int64_t>(
                        ((fsouth->getMap().count("rotation") ? fsouth->getMap()["rotation"]->getNumber() : 0) +
                         (lockuv ? -90 : 90)) %
                        360));
                }
            };

            int txrot = xrot;
            int tyrot = yrot;
            int tzrot = zrot;
            while (txrot > 0)
            {
                xrot90();
                txrot -= 90;
            }
            while (tyrot > 0)
            {
                yrot90();
                tyrot -= 90;
            }
            while (tzrot > 0)
            {
                zrot90();
                tzrot -= 90;
            }

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
    return std::abs(n->getNumberFloating() - std::round(n->getNumberFloating())) > 1e-5;
}

auto OMModelPrecompiler::precompile(OMIdentifier name, bool subsitute)
    -> std::shared_ptr<openminecraft::io::json::OMJsonNode>
{
    auto ff = vfs::fsfetch(fmt::format("{}/{}/models/{}.json", root, name.namesp, name.path));
    json::OMJsonAstBuilder bld(std::make_shared<json::OMJsonTokenIter>(ff));
    auto ll = bld.build();

    if (ll->getMap().count("parent"))
    {
        auto sub = precompile(OMIdentifier(ll->getMap()["parent"]->getString()), false);
        ll->getMap().erase("parent");
        ll->merge(sub);
    }

    if (subsitute)
    {
        auto &tex = ll->getMap()["textures"]->getMap();
        for (auto &pp : tex)
        {
            if (pp.second->getString()[0] == '#' && tex.count(pp.second->getString().substr(1)))
            {
                pp.second = tex[pp.second->getString().substr(1)];
            }
        }

        if (ll->getMap().count("elements"))
        {
            for (auto &elem : ll->getMap()["elements"]->getArray())
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
                        auto aa = fce.second->getMap()["uv"]->getArray();

                        if (isNotInteger(aa[0]) || isNotInteger(aa[1]) || isNotInteger(aa[2]) || isNotInteger(aa[3]))
                        {
                            modelComplex[modelId] = true;
                        }
                    }
                }
            }
        }
    }

    return ll;
}

auto OMModelPrecompiler::wrapPart(std::shared_ptr<openminecraft::io::json::OMJsonNode> part) -> OMModelPart
{
    auto mm = part->getMap()["faces"]->getMap();
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

    return {mm.count("east") ? wrapFace(mm["east"], OMModelCullSide::East, fromv, tov) : OMModelFace(),
            mm.count("east") > 0,
            mm.count("west") ? wrapFace(mm["west"], OMModelCullSide::West, fromv, tov) : OMModelFace(),
            mm.count("west") > 0,
            mm.count("down") ? wrapFace(mm["down"], OMModelCullSide::Down, fromv, tov) : OMModelFace(),
            mm.count("down") > 0,
            mm.count("up") ? wrapFace(mm["up"], OMModelCullSide::Up, fromv, tov) : OMModelFace(),
            mm.count("up") > 0,
            mm.count("south") ? wrapFace(mm["south"], OMModelCullSide::South, fromv, tov) : OMModelFace(),
            mm.count("south") > 0,
            mm.count("north") ? wrapFace(mm["north"], OMModelCullSide::North, fromv, tov) : OMModelFace(),
            mm.count("north") > 0,
            fromv,
            tov,
            part->getMap().count("shade") ? part->getMap()["shade"]->getBoolean() : true,
            ro != nullptr,
            roCenter,
            ro && ro->getMap()["axis"] != nullptr ? fromAxis(ro->getMap()["axis"]->getString()) : (ro ? Any : X),
            ro && ro->getMap()["angle"] != nullptr ? ro->getMap()["angle"]->getNumberFloating() : 0.0,
            roAngleExt};
}

auto OMModelPrecompiler::wrapFace(std::shared_ptr<openminecraft::io::json::OMJsonNode> face, OMModelCullSide c,
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
            textureAtlas.subtex.count(ident) == 0};
}

auto OMModelPrecompiler::composeBlock(std::vector<int> partids, bool soild) -> int
{
    auto &m = blockModels.emplace_back();
    m.soild = soild;
    for (auto i : partids)
    {
        for (auto &mp : modelParts[i])
        {
            m.parts.push_back(mp);
            m.partComplex.push_back(modelComplex[i]);
            m.partAmbientOcculusion.push_back(modelAmbientOcculusion[i]);
        }
    }

    return blockModels.size() - 1;
}
} // namespace openminecraftshell::data