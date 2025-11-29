#ifndef OM_VFS_BASE_HPP
#define OM_VFS_BASE_HPP

#include "openminecraft/specs/vfsbundle/om_vfsbundle.hpp"

#include <functional>
#include <istream>
#include <memory>
#include <string>
#include <variant>

namespace openminecraft::vfs
{
enum MountType
{
    Real,
    Assets,
    Bundle
};
struct MountInfo
{
    MountType type;
    std::variant<std::string, std::shared_ptr<specs::vfsbundle::OMBundle>> info;
};
extern std::unordered_map<std::string, std::function<std::shared_ptr<std::istream>(std::string)>> m;
extern std::unordered_map<std::string, MountInfo> info;
bool fsmountReal(std::string path, std::string mountpoint);
bool fsmountAssets(std::string mountpoint);
bool fsmountBundle(std::shared_ptr<specs::vfsbundle::OMBundle> info, std::string mountpoint);
bool fsumount(std::string mountpoint);
std::shared_ptr<std::istream> fsfetch(std::string fullPath);
} // namespace openminecraft::vfs

#endif