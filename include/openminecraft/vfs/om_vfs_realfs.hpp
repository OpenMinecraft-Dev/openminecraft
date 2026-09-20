#ifndef OM_VFS_REALFS_HPP
#define OM_VFS_REALFS_HPP

#include "openminecraft/vfs/om_vfs_base.hpp"
namespace openminecraft::vfs
{
class OMFsProviderReal : public OMFsProvider
{
  public:
    OMFsProviderReal(std::string root) : root(root)
    {
    }

    auto read(std::string) -> std::shared_ptr<std::istream> override;
    auto write(std::string) -> std::shared_ptr<std::ostream> override;

  private:
    std::string root;
};
} // namespace openminecraft::vfs

#endif