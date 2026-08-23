#include "openminecraft-shell/data/block/om_blockstate_resolver.hpp"
#include "openminecraft-shell/data/block/om_block.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"

using namespace openminecraft::io;

namespace openminecraftshell::data::block
{
void OMBlockstateResolver::resolveModel(OMBlock &blk, std::shared_ptr<io::json::OMJsonNode> node)
{
    OMIdentifier ident(node->getMap()["model"]->getString());
    int xrot = node->getMap().count("x") ? node->getMap()["x"]->getNumber() : 0;
    int yrot = node->getMap().count("y") ? node->getMap()["y"]->getNumber() : 0;
    int zrot = node->getMap().count("z") ? node->getMap()["z"]->getNumber() : 0;
    bool uvlock = node->getMap().count("uvlock") ? node->getMap()["uvlock"]->getBoolean() : false;
    OMBlockModelIdentifier i = {ident, xrot, yrot, zrot, uvlock};
    if (blk.requiredModels.count(i))
    {
        return;
    }
    blk.requiredModels[i] = compiler.loadModelPartWithArgs(ident, xrot, yrot, zrot, uvlock);
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
}
} // namespace openminecraftshell::data::block