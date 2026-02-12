#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

namespace openminecraft::vfs
{
std::unordered_map<std::string, std::function<std::shared_ptr<std::istream>(std::string)>> m;
std::unordered_map<std::string, MountInfo> info;
log::OMLogger logger("vfs");
bool mountinvaild(std::string mp)
{
    return mp.empty() || mp[0] != '/' || mp == "/" || mp[mp.length() - 1] == '/';
}
bool fsmountReal(std::string path, std::string mountpoint)
{
    if (!std::filesystem::exists(path))
    {
        logger.info("!real:{}", path);
        return false;
    }
    if (mountinvaild(mountpoint))
    {
        return false;
    }
    m[mountpoint] = [path](std::string proc) -> std::shared_ptr<std::istream> {
        return std::make_shared<std::ifstream>(path + "/" + proc, std::ios::binary);
    };
    info[mountpoint] = {Real, path};
    logger.info("real:{} -> virt:{}", path, mountpoint);
    return true;
}
bool fsumount(std::string mountpoint)
{
    if (m.count(mountpoint))
    {
        m.erase(mountpoint);
        info.erase(mountpoint);
        logger.info("null -> virt:{}", mountpoint);
        return true;
    }

    return false;
}
bool fsmountBundle(std::shared_ptr<specs::vfsbundle::OMBundle> info, std::string mountpoint)
{
    if (mountinvaild(mountpoint))
    {
        return false;
    }
    m[mountpoint] = [info](std::string proc) -> std::shared_ptr<std::istream> {
        for (auto i : info->files)
        {
            logger.info("{} {}", proc, i.first.name);
            if (i.first.name == proc)
            {
                return std::make_shared<std::istringstream>(
                    std::string(reinterpret_cast<char *>(i.second), i.first.length));
            }
        }
        return nullptr;
    };
    vfs::info[mountpoint] = {Bundle, info};
    logger.info("bundle -> virt:{}", mountpoint);
    return false;
}
std::string compressPath(std::string vp)
{
    std::vector<std::string> pathsegs;
    std::vector<int> slash;
    int i = 0;
    for (auto c : vp)
    {
        if (c == '/')
        {
            slash.push_back(i);
        }
        i++;
    }
    slash.push_back(vp.length());

    for (int i = 0; i < slash.size() - 1; i++)
    {
        pathsegs.push_back(std::string(vp.substr(slash[i] + 1, slash[i + 1] - slash[i] - 1)));
    }

    std::vector<std::string> proc;
    for (auto m : pathsegs)
    {
        if (m == ".")
        {
        }
        else if (m == ".." && !proc.empty())
        {
            proc.erase(proc.end() - 1);
        }
        else
        {
            proc.push_back(m);
        }
    }

    std::string target;
    for (auto m : proc)
    {
        target.append("/").append(m);
    }

    return target;
}
std::shared_ptr<std::istream> fsfetch(std::string fullPath)
{
    auto pth = compressPath(fullPath);
    for (auto p : m)
    {
        if (!pth.find(p.first))
        {
            return p.second(pth.substr(p.first.length() + 1, pth.length()));
        }
    }
    return std::shared_ptr<std::istream>(nullptr);
}
} // namespace openminecraft::vfs
