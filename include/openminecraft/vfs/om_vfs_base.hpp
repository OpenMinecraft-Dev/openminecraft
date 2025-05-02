#ifndef OM_VFS_BASE_HPP
#define OM_VFS_BASE_HPP

#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <string>

namespace openminecraft::vfs
{
extern std::map<std::string, std::function<std::shared_ptr<std::istream>(std::string)>> m;
bool fsmountReal(std::string path, std::string mountpoint);
bool fsmountAssets(std::string mountpoint);
bool fsmountBundle(void *data, size_t length, std::string mountpoint);
bool fsumount(std::string mountpoint);
std::shared_ptr<std::istream> fsfetch(std::string fullPath);
} // namespace openminecraft::vfs

#endif