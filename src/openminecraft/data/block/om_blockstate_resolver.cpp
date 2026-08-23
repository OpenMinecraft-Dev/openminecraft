#include "openminecraft-shell/data/block/om_blockstate_resolver.hpp"
#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/block/om_blockstate.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"

using namespace openminecraft::io;

namespace openminecraftshell::data::block
{
auto OMBlockstateResolver::identFrom(std::shared_ptr<io::json::OMJsonNode> node) -> OMBlockModelIdentifier
{
    OMIdentifier ident(node->getMap()["model"]->getString());
    int xrot = node->getMap().count("x") ? node->getMap()["x"]->getNumber() : 0;
    int yrot = node->getMap().count("y") ? node->getMap()["y"]->getNumber() : 0;
    int zrot = node->getMap().count("z") ? node->getMap()["z"]->getNumber() : 0;
    bool uvlock = node->getMap().count("uvlock") ? node->getMap()["uvlock"]->getBoolean() : false;
    return {ident, xrot, yrot, zrot, uvlock};
}
void OMBlockstateResolver::resolveModel(OMBlock &blk, std::shared_ptr<io::json::OMJsonNode> node)
{
    auto i = identFrom(node);
    if (blk.requiredModels.count(i))
    {
        return;
    }
    blk.requiredModels[i] = compiler.loadModelPartWithArgs(i.location, i.xrot, i.yrot, i.zrot, i.uvlock);
}
void OMBlockstateResolver::resolve(OMBlock &blk)
{
    blk.states.clear();
    blk.requiredModels.clear();

    auto ident = blk.name;
    auto ff = vfs::fsfetch(fmt::format("{}/{}/blockstates/{}.json", root, ident.namesp, ident.path));
    json::OMJsonAstBuilder bld(std::make_shared<json::OMJsonTokenIter>(ff));
    auto ll = bld.build();

    if (ll->getMap().count("variants"))
    {
        for (auto &l : ll->getMap()["variants"]->getMap())
        {
            resolveModel(blk, l.second);
        }
    }
    else if (ll->getMap().count("multipart"))
    {
        for (auto &l : ll->getMap()["multipart"]->getArray())
        {
            resolveModel(blk, l->getMap()["apply"]);
        }
    }
    blk.resolverCache = ll;
}

auto OMBlockstateResolver::fetchModel(OMBlock &blk, std::string state) -> int
{
    auto st = OMBlockState(state);
    auto hsh = st.hash();

    if (blk.states.count(st))
    {
        return blk.states[st];
    }

    if (blk.resolverCache->getMap().count("variants"))
    {
        for (auto &var : blk.resolverCache->getMap()["variants"]->getMap())
        {
            if (OMBlockState(var.first).hash() == hsh)
            {
                auto i = compiler.composeBlock({}, blk.soild);
                blk.states[st] = i;
                return i;
            }
        }
    }

    return -1;
}
} // namespace openminecraftshell::data::block