#include <iostream>
#include <utility>

#include "openminecraft-shell/data/om_model_precompiler.hpp"
#include "fmt/format.h"
#include "glm/fwd.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"
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

auto OMModelPrecompiler::queryPartSize(int bsid, int pid) -> glm::ivec3
{
    return models[bsid][pid].to - models[bsid][pid].from;
}
auto OMModelPrecompiler::queryPartOffset(int bsid, int pid) -> glm::ivec3
{
    return models[bsid][pid].from;
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

auto OMModelPrecompiler::queryPartFaceUVAuto(int bsid, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing f)
    -> bool
{
    switch (f)
    {
    default:
    case ::NegX:
        return models[bsid][pid].west.autoUV;
    case ::PosX:
        return models[bsid][pid].east.autoUV;
    case ::NegZ:
        return models[bsid][pid].north.autoUV;
    case ::PosZ:
        return models[bsid][pid].south.autoUV;
    case ::NegY:
        return models[bsid][pid].down.autoUV;
    case ::PosY:
        return models[bsid][pid].up.autoUV;
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

auto OMModelPrecompiler::loadModel(OMIdentifier i) -> int
{
    models.resize(modelId + 1);
    models[modelId] = {};

    auto pre = precompile(i);

    if (pre)
    {
        for (auto &p : pre->getArray())
        {
            models[modelId].emplace_back(wrapPart(p));
        }
    }

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
        return ll->getMap()["elements"];
    }
    else
    {
        return ll;
    }
}

auto OMModelPrecompiler::wrapPart(std::shared_ptr<openminecraft::io::json::OMJsonNode> part) -> OMModelPart
{
    auto mm = part->getMap()["faces"]->getMap();
    auto from = part->getMap()["from"]->getArray();
    auto to = part->getMap()["to"]->getArray();
    auto ro = part->getMap().count("rotation") ? part->getMap()["rotation"] : nullptr;
    return {
        mm.count("east") ? wrapFace(mm["east"]) : OMModelFace(),
        mm.count("east") > 0,
        mm.count("west") ? wrapFace(mm["west"]) : OMModelFace(),
        mm.count("west") > 0,
        mm.count("down") ? wrapFace(mm["down"]) : OMModelFace(),
        mm.count("down") > 0,
        mm.count("up") ? wrapFace(mm["up"]) : OMModelFace(),
        mm.count("up") > 0,
        mm.count("south") ? wrapFace(mm["south"]) : OMModelFace(),
        mm.count("south") > 0,
        mm.count("north") ? wrapFace(mm["north"]) : OMModelFace(),
        mm.count("north") > 0,
        {from[0]->getNumber(), from[1]->getNumber(), from[2]->getNumber()},
        {to[0]->getNumber(), to[1]->getNumber(), to[2]->getNumber()},
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

auto OMModelPrecompiler::wrapFace(std::shared_ptr<openminecraft::io::json::OMJsonNode> face) -> OMModelFace
{
    auto uv = face->getMap().count("uv") ? face->getMap()["uv"] : nullptr;
    return {
        from(face->getMap().count("cullface") ? face->getMap()["cullface"]->getString() : "none"),
        textureAtlas.subtex[OMIdentifier(face->getMap()["texture"]->getString())],
        {uv ? uv->getArray()[0]->getNumber() : 0, uv ? uv->getArray()[1]->getNumber() : 0},
        {uv ? uv->getArray()[2]->getNumber() : 0, uv ? uv->getArray()[3]->getNumber() : 0},
        uv == nullptr,
        true,
        face->getMap().count("rotation") ? static_cast<int>(face->getMap()["rotation"]->getNumber()) : 0,
    };
}
} // namespace openminecraftshell::data