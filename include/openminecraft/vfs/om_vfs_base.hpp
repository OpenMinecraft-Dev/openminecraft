#ifndef OM_VFS_BASE_HPP
#define OM_VFS_BASE_HPP

#include <functional>
#include <istream>
#include <memory>
#include <string>
#include <variant>

namespace openminecraft::vfs
{
struct BundleInfo
{
    void *p;
    size_t length;
};
enum MountType
{
    Real,
    Assets,
    Bundle
};
struct MountInfo
{
    MountType type;
    std::variant<std::string, BundleInfo> info;
};
extern std::unordered_map<std::string, std::function<std::shared_ptr<std::istream>(std::string)>> m;
extern std::unordered_map<std::string, MountInfo> info;
bool fsmountReal(std::string path, std::string mountpoint);
bool fsmountAssets(std::string mountpoint);
bool fsmountBundle(BundleInfo info, std::string mountpoint);
bool fsumount(std::string mountpoint);
std::shared_ptr<std::istream> fsfetch(std::string fullPath);
} // namespace openminecraft::vfs

#endif