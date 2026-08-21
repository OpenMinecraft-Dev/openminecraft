#include <iostream>
#include <memory>
#include <utility>

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
auto OMModelPrecompiler::queryPartShade(int bsid, int pid) -> bool
{
    return models[bsid][pid].shade;
}
auto OMModelPrecompiler::queryAmbientOcclusion(int bsid) -> bool
{
    return modelAmbientOcculusion[bsid];
}

auto OMModelPrecompiler::queryOcclusionShape(int bsid) -> OMVoxelShape
{
    std::vector<OMVoxelAABB> r;
    for (auto &pp : models[bsid])
    {
        r.emplace_back(OMVoxelAABB{pp.from, pp.to - pp.from});
    }

    return {r};
}

auto OMModelPrecompiler::queryPartAABB(int bsid, int pid) -> OMVoxelAABB
{
    return {models[bsid][pid].from, models[bsid][pid].to - models[bsid][pid].from};
}

auto OMModelPrecompiler::queryNumParts(int bsid) -> int
{
    return models[bsid].size();
}
auto OMModelPrecompiler::queryPartFaceEnabled(int bsid, int pid, ::OMVoxelFacing f) -> bool
{
    switch (f)
    {
    default:
    case ::NegX:
        return models[bsid][pid].enableWest;
    case ::PosX:
        return models[bsid][pid].enableEast;
    case ::NegZ:
        return models[bsid][pid].enableNorth;
    case ::PosZ:
        return models[bsid][pid].enableSouth;
    case ::NegY:
        return models[bsid][pid].enableDown;
    case ::PosY:
        return models[bsid][pid].enableUp;
    }
}
auto OMModelPrecompiler::queryPartFaceTex(int bsid, int pid, ::OMVoxelFacing f) -> int
{
    switch (f)
    {
    default:
    case ::NegX:
        return models[bsid][pid].west.textureid;
    case ::PosX:
        return models[bsid][pid].east.textureid;
    case ::NegZ:
        return models[bsid][pid].north.textureid;
    case ::PosZ:
        return models[bsid][pid].south.textureid;
    case ::NegY:
        return models[bsid][pid].down.textureid;
    case ::PosY:
        return models[bsid][pid].up.textureid;
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
        return cullFaceTo(models[bsid][pid].west.cull);
    case ::PosX:
        return cullFaceTo(models[bsid][pid].east.cull);
    case ::NegZ:
        return cullFaceTo(models[bsid][pid].north.cull);
    case ::PosZ:
        return cullFaceTo(models[bsid][pid].south.cull);
    case ::NegY:
        return cullFaceTo(models[bsid][pid].down.cull);
    case ::PosY:
        return cullFaceTo(models[bsid][pid].up.cull);
    }
}

auto OMModelPrecompiler::queryPartFaceUV(int bsid, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing f)
    -> glm::ivec4
{
    glm::ivec2 uv0, uv1;
    switch (f)
    {
    default:
    case ::NegX:
        uv0 = models[bsid][pid].west.uv0;
        uv1 = models[bsid][pid].west.uv1;
        break;
    case ::PosX:
        uv0 = models[bsid][pid].east.uv0;
        uv1 = models[bsid][pid].east.uv1;
        break;
    case ::NegZ:
        uv0 = models[bsid][pid].north.uv0;
        uv1 = models[bsid][pid].north.uv1;
        break;
    case ::PosZ:
        uv0 = models[bsid][pid].south.uv0;
        uv1 = models[bsid][pid].south.uv1;
        break;
    case ::NegY:
        uv0 = models[bsid][pid].down.uv0;
        uv1 = models[bsid][pid].down.uv1;
        break;
    case ::PosY:
        uv0 = models[bsid][pid].up.uv0;
        uv1 = models[bsid][pid].up.uv1;
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
        return models[bsid][pid].west.rotation / 90;
    case ::PosX:
        return models[bsid][pid].east.rotation / 90;
    case ::NegZ:
        return models[bsid][pid].north.rotation / 90;
    case ::PosZ:
        return models[bsid][pid].south.rotation / 90;
    case ::NegY:
        return models[bsid][pid].down.rotation / 90;
    case ::PosY:
        return models[bsid][pid].up.rotation / 90;
    }
}

auto OMModelPrecompiler::queryPartRotationAxis(int bsid, int pid) -> int
{
    return models[bsid][pid].rotateAxis;
}

auto OMModelPrecompiler::queryPartRotationCenter(int bsid, int pid) -> glm::ivec3
{
    return models[bsid][pid].rotateOrigin;
}

auto OMModelPrecompiler::queryPartRotationAngle(int bsid, int pid) -> int
{
    return static_cast<float>(models[bsid][pid].rotateAngle + 45) / 22.5f;
}

auto OMModelPrecompiler::querySoild(int bsid) -> bool
{
    return modelSoild[bsid];
}

auto OMModelPrecompiler::loadModel(OMIdentifier i, bool soild) -> int
{
    return loadModelWithArgs(i, 0, 0, 0, false, soild);
}

auto rotateVoxelElementZ90(const glm::ivec3 &from, const glm::ivec3 &to) -> std::pair<glm::ivec3, glm::ivec3>
{
    glm::ivec3 corners[8] = {{from.x, from.y, from.z}, {from.x, from.y, to.z}, {from.x, to.y, from.z},
                             {from.x, to.y, to.z},     {to.x, from.y, from.z}, {to.x, from.y, to.z},
                             {to.x, to.y, from.z},     {to.x, to.y, to.z}};

    glm::ivec3 newMin = glm::ivec3(16, 16, 16);
    glm::ivec3 newMax = glm::ivec3(0, 0, 0);

    for (const auto &p : corners)
    {
        glm::ivec3 centered = p - glm::ivec3(8, 8, 8);

        glm::ivec3 rotated;
        rotated = {-centered.y, centered.x, centered.z};
        glm::ivec3 finalPos = rotated + glm::ivec3(8, 8, 8);

        newMin = glm::min(newMin, finalPos);
        newMax = glm::max(newMax, finalPos);
    }

    return {newMin, newMax};
}

auto rotateVoxelElementX90(const glm::ivec3 &from, const glm::ivec3 &to) -> std::pair<glm::ivec3, glm::ivec3>
{
    glm::ivec3 corners[8] = {{from.x, from.y, from.z}, {from.x, from.y, to.z}, {from.x, to.y, from.z},
                             {from.x, to.y, to.z},     {to.x, from.y, from.z}, {to.x, from.y, to.z},
                             {to.x, to.y, from.z},     {to.x, to.y, to.z}};

    glm::ivec3 newMin = glm::ivec3(16, 16, 16);
    glm::ivec3 newMax = glm::ivec3(0, 0, 0);

    for (const auto &p : corners)
    {
        glm::ivec3 centered = p - glm::ivec3(8, 8, 8);

        glm::ivec3 rotated;
        rotated = {centered.x, -centered.z, centered.y};
        glm::ivec3 finalPos = rotated + glm::ivec3(8, 8, 8);

        newMin = glm::min(newMin, finalPos);
        newMax = glm::max(newMax, finalPos);
    }

    return {newMin, newMax};
}

auto rotateVoxelElementY90(const glm::ivec3 &from, const glm::ivec3 &to) -> std::pair<glm::ivec3, glm::ivec3>
{
    glm::ivec3 corners[8] = {{from.x, from.y, from.z}, {from.x, from.y, to.z}, {from.x, to.y, from.z},
                             {from.x, to.y, to.z},     {to.x, from.y, from.z}, {to.x, from.y, to.z},
                             {to.x, to.y, from.z},     {to.x, to.y, to.z}};

    glm::ivec3 newMin = glm::ivec3(16, 16, 16);
    glm::ivec3 newMax = glm::ivec3(0, 0, 0);

    for (const auto &p : corners)
    {
        glm::ivec3 centered = p - glm::ivec3(8, 8, 8);

        glm::ivec3 rotated;
        rotated = {-centered.z, centered.y, centered.x};
        glm::ivec3 finalPos = rotated + glm::ivec3(8, 8, 8);

        newMin = glm::min(newMin, finalPos);
        newMax = glm::max(newMax, finalPos);
    }

    return {newMin, newMax};
}

auto OMModelPrecompiler::loadModelWithArgs(OMIdentifier i, int xrot, int yrot, int zrot, bool lockuv, bool soild) -> int
{
    models.resize(modelId + 1);
    modelSoild.resize(modelId + 1);
    modelAmbientOcculusion.resize(modelId + 1);
    models[modelId] = {};

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
                    mm[f] = a;
                }
            };
            auto xrot90 = [&]() {
                auto &from = p->getMap()["from"]->getArray();
                auto &to = p->getMap()["to"]->getArray();
                auto fromv = glm::ivec3{from[0]->getNumber(), from[1]->getNumber(), from[2]->getNumber()};
                auto tov = glm::ivec3{to[0]->getNumber(), to[1]->getNumber(), to[2]->getNumber()};
                auto vv = rotateVoxelElementX90(fromv, tov);

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
                auto fromv = glm::ivec3{from[0]->getNumber(), from[1]->getNumber(), from[2]->getNumber()};
                auto tov = glm::ivec3{to[0]->getNumber(), to[1]->getNumber(), to[2]->getNumber()};
                auto vv = rotateVoxelElementY90(fromv, tov);

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
                auto fromv = glm::ivec3{from[0]->getNumber(), from[1]->getNumber(), from[2]->getNumber()};
                auto tov = glm::ivec3{to[0]->getNumber(), to[1]->getNumber(), to[2]->getNumber()};
                auto vv = rotateVoxelElementZ90(fromv, tov);

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

            while (xrot > 0)
            {
                xrot90();
                xrot -= 90;
            }
            while (yrot > 0)
            {
                yrot90();
                yrot -= 90;
            }
            while (zrot > 0)
            {
                zrot90();
                zrot -= 90;
            }

            models[modelId].emplace_back(wrapPart(p));
        }
    }

    modelSoild[modelId] = soild;
    modelId++;
    return modelId - 1;
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
                for (auto &fce : elem->getMap()["faces"]->getMap())
                {
                    auto text = fce.second->getMap()["texture"]->getString();

                    if (text[0] == '#' && tex.count(text.substr(1)))
                    {
                        fce.second->getMap()["texture"] = tex[text.substr(1)];
                        textureAtlas.addTexture(OMIdentifier(fce.second->getMap()["texture"]->getString()));
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
    auto fromv = glm::ivec3{from[0]->getNumber(), from[1]->getNumber(), from[2]->getNumber()};
    auto tov = glm::ivec3{to[0]->getNumber(), to[1]->getNumber(), to[2]->getNumber()};

    return {
        mm.count("east") ? wrapFace(mm["east"], OMModelCullSide::East, fromv, tov) : OMModelFace(),
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
        ro ? glm::ivec3{static_cast<int>(ro->getMap()["origin"]->getArray()[0]->getNumber()),
                        static_cast<int>(ro->getMap()["origin"]->getArray()[1]->getNumber()),
                        static_cast<int>(ro->getMap()["origin"]->getArray()[2]->getNumber())}
           : glm::ivec3{},
        ro ? fromAxis(ro->getMap()["axis"]->getString()) : X,
        ro ? ro->getMap()["angle"]->getNumberFloating() : 0.0,
    };
}

auto OMModelPrecompiler::wrapFace(std::shared_ptr<openminecraft::io::json::OMJsonNode> face, OMModelCullSide c,
                                  glm::ivec3 from, glm::ivec3 to) -> OMModelFace
{
    auto uv = face->getMap().count("uv") ? face->getMap()["uv"] : nullptr;
    glm::ivec2 uv0, uv1;
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

    auto mapped = textureAtlas.mapTexture(OMIdentifier(face->getMap()["texture"]->getString()), uv0, uv1);

    return {
        fromSide(face->getMap().count("cullface") ? face->getMap()["cullface"]->getString() : "none"),
        std::get<2>(mapped),
        std::get<0>(mapped),
        std::get<1>(mapped),
        face->getMap().count("rotation") ? static_cast<int>(face->getMap()["rotation"]->getNumber()) : 0,
    };
}
} // namespace openminecraftshell::data