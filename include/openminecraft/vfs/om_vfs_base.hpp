#ifndef OM_VFS_BASE_HPP
#define OM_VFS_BASE_HPP

#include "openminecraft/specs/vfsbundle/om_vfsbundle.hpp"

#include <cstddef>
#include <istream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>

namespace openminecraft::vfs
{
class OMFsProvider
{
  public:
    virtual auto read(std::string) -> std::shared_ptr<std::istream> = 0;
    virtual auto write(std::string) -> std::shared_ptr<std::ostream>
    {
        throw std::logic_error("not supported");
    }
};

extern std::unordered_map<std::string, std::shared_ptr<OMFsProvider>> fshandlers;

auto fsmountReal(std::string root, std::string mountpoint) -> bool;
auto fsmountBundle(std::shared_ptr<specs::vfsbundle::OMBundle> info, std::string mountpoint) -> bool;
auto fsmountZipArchive(const char *src, std::size_t length, std::string mountpoint) -> bool;
auto fsmountZipArchive(std::shared_ptr<std::istream> istr, std::string mountpoint) -> bool;
auto fsumount(std::string mountpoint) -> bool;
auto fsfetch(std::string fullPath) -> std::shared_ptr<std::istream>;
} // namespace openminecraft::vfs

#endif
