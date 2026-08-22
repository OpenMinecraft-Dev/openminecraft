#include "openminecraft-shell/data/block/om_blockstate_resolver.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/json/om_io_ast_builder_json.hpp"

using namespace openminecraft::io;

namespace openminecraftshell::data::block
{
void OMBlockstateResolver::resolve(OMBlock &blk)
{
    auto ident = blk.name;
    auto ff = vfs::fsfetch(fmt::format("{}/{}/blockstates/{}.json", root, ident.namesp, ident.path));
    json::OMJsonAstBuilder bld(std::make_shared<json::OMJsonTokenIter>(ff));
    auto ll = bld.build();

    if (ll->getMap().count("variants"))
    {
        for (auto &l : ll->getMap()["variants"]->getMap())
        {
            logger.warn("{}", l.first);
        }
    }
    else
    {
        logger.warn("multipart not supported!");
    }
}
} // namespace openminecraftshell::data::block