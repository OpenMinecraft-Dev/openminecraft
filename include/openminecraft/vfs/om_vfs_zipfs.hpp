#ifndef OM_VFS_ZIPFS_HPP
#define OM_VFS_ZIPFS_HPP

#include "openminecraft/specs/zip/om_zip.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <memory>
namespace openminecraft::vfs
{
class OMFsProviderZip : public OMFsProvider
{
  public:
    OMFsProviderZip(std::shared_ptr<specs::zip::OMZip> file) : file(file)
    {
    }

    auto read(std::string) -> std::shared_ptr<std::istream> override;

  private:
    std::shared_ptr<specs::zip::OMZip> file;
};
} // namespace openminecraft::vfs

#endif