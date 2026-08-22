#include "openminecraft-shell/data/block/om_blockstate_resolver.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

namespace openminecraftshell::data::block
{
void OMBlockstateResolver::resolve(OMBlock &blk)
{
    auto ident = blk.name;
    auto ff = vfs::fsfetch(fmt::format("{}/{}/blockstates/{}.json", root, ident.namesp, ident.path));
    logger.debug("resolving {}", (void *)ff.get());
}
} // namespace openminecraftshell::data::block