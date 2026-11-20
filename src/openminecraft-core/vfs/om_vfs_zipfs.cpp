#include "openminecraft/vfs/om_vfs_zipfs.hpp"

namespace openminecraft::vfs
{
auto OMFsProviderZip::read(std::string proc) -> std::shared_ptr<std::istream>
{
    auto handle = file->findFile(proc);
    if (!handle)
    {
        return nullptr;
    }
    return file->read(handle);
}
} // namespace openminecraft::vfs