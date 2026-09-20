#ifndef OM_VFS_BUNDLEFS_HPP
#define OM_VFS_BUNDLEFS_HPP

#include "openminecraft/specs/vfsbundle/om_vfsbundle.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
namespace openminecraft::vfs
{
class OMFsProviderBundle : public OMFsProvider
{
  public:
    OMFsProviderBundle(std::shared_ptr<specs::vfsbundle::OMBundle> file) : file(file)
    {
    }

    auto read(std::string) -> std::shared_ptr<std::istream> override;

  private:
    std::shared_ptr<specs::vfsbundle::OMBundle> file;
};
} // namespace openminecraft::vfs

#endif