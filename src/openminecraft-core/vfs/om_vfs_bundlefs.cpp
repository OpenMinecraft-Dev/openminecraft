#include "openminecraft/vfs/om_vfs_bundlefs.hpp"
#include "openminecraft/util/om_util_memstream.hpp"

namespace openminecraft::vfs
{
auto OMFsProviderBundle::read(std::string proc) -> std::shared_ptr<std::istream>
{
    for (auto i : file->files)
    {
        if (i.first.name == proc)
        {
            return std::make_shared<util::OMMemoryStream>(reinterpret_cast<const char *>(i.second), i.first.length);
        }
    }
    return nullptr;
}
} // namespace openminecraft::vfs