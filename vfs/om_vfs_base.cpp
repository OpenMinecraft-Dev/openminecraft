#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <fstream>
#include <functional>
#include <istream>
#include <map>
#include <memory>

namespace openminecraft::vfs
{
std::map<std::string, std::function<std::shared_ptr<std::istream>(std::string)>> m;
log::OMLogger logger("vfs");
bool fsmountReal(std::string path, std::string mountpoint)
{
    if (mountpoint.empty() || mountpoint[0] != '/' || mountpoint == "/" || mountpoint[mountpoint.length() - 1] == '/')
    {
        return false;
    }
    m[mountpoint] = [path](std::string proc) -> std::shared_ptr<std::istream> {
        return std::make_shared<std::ifstream>(path + "/" + proc, std::ios::binary);
    };
    return true;
}
bool fsumount(std::string mountpoint)
{
    if (m.count(mountpoint))
    {
        m.erase(mountpoint);
        return true;
    }
    else
    {
        return false;
    }
    return false;
}
std::shared_ptr<std::istream> fsfetch(std::string fullPath)
{
    for (auto p : m)
    {
        if (!fullPath.find(p.first))
        {
            logger.info("{}", fullPath.substr(p.first.length(), fullPath.length()));
            return p.second(fullPath.substr(p.first.length(), fullPath.length()));
        }
    }
    return std::shared_ptr<std::istream>(nullptr);
}
} // namespace openminecraft::vfs