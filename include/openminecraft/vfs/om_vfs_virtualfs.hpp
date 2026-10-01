#ifndef OM_VFS_VIRTUALFS_HPP
#define OM_VFS_VIRTUALFS_HPP

#include "openminecraft/vfs/om_vfs_base.hpp"
#include <memory>
#include <ostream>
#include <sstream>
#include <unordered_map>
namespace openminecraft::vfs
{
class OMFsProviderVirtual : public OMFsProvider
{
  public:
    auto read(std::string p) -> std::shared_ptr<std::istream> override
    {
        if (overwrites.count(p))
        {
            return std::make_shared<std::istringstream>(overwrites[p]->str());
        }
        return nullptr;
    }
    auto write(std::string p) -> std::shared_ptr<std::ostream> override
    {
        if (overwrites.count(p))
        {
            return overwrites[p];
        }

        overwrites[p] = std::make_shared<std::ostringstream>();
        return overwrites[p];
    }

  private:
    std::unordered_map<std::string, std::shared_ptr<std::ostringstream>> overwrites;
};
} // namespace openminecraft::vfs

#endif