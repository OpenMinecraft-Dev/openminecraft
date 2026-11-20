#include "openminecraft/vfs/om_vfs_realfs.hpp"
#include <istream>
#include <fstream>
#include <memory>

namespace openminecraft::vfs
{
auto OMFsProviderReal::read(std::string proc) -> std::shared_ptr<std::istream>
{
    return std::make_shared<std::ifstream>(root + "/" + proc, std::ios::binary);
}
auto OMFsProviderReal::write(std::string proc) -> std::shared_ptr<std::ostream>
{
    return std::make_shared<std::ofstream>(root + "/" + proc, std::ios::binary);
}
} // namespace openminecraft::vfs