#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <fstream>
#include <istream>
#include <map>
#include <memory>

namespace openminecraft::vfs
{
std::map<std::string, std::string> m;
log::OMLogger logger("test");
bool fsmountReal(std::string path, std::string mountpoint)
{
    if (mountpoint[0] != '/' || mountpoint[mountpoint.length() - 1] == '/')
    {
        return false;
    }
    m[mountpoint] = path;
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
}
std::shared_ptr<std::istream> fsfetch(std::string fullPath)
{
    for (auto p : m)
    {
        if (!fullPath.find(p.first))
        {
            fullPath.replace(0, p.first.length(), p.second);
            return std::make_shared<std::ifstream>(fullPath, std::ios::binary);
        }
    }
    return std::shared_ptr<std::istream>(nullptr);
}
} // namespace openminecraft::vfs