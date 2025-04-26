#ifndef OM_VFS_BASE_HPP
#define OM_VFS_BASE_HPP

#include <istream>
#include <memory>
#include <string>

namespace openminecraft::vfs
{
bool fsmountReal(std::string path, std::string mountpoint);
bool fsumount(std::string mountpoint);
std::shared_ptr<std::istream> fsfetch(std::string fullPath);
} // namespace openminecraft::vfs

#endif