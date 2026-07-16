#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/specs/zip/om_zip.hpp"
#include "openminecraft/util/om_util_memstream.hpp"
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <istream>
#include <memory>
#include <string>
#include <unordered_map>

namespace openminecraft::vfs
{
std::unordered_map<std::string, std::function<std::shared_ptr<std::istream>(std::string)>> m;
std::unordered_map<std::string, MountInfo> info;
log::OMLogger logger("vfs");
auto mountinvaild(std::string mp) -> bool
{
    return mp.empty() || mp[0] != '/' || mp == "/" || mp[mp.length() - 1] == '/';
}
auto fsmountReal(std::string path, std::string mountpoint) -> bool
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
auto fsumount(std::string mountpoint) -> bool
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
auto fsmountBundle(std::shared_ptr<specs::vfsbundle::OMBundle> info, std::string mountpoint) -> bool
{
    if (mountinvaild(mountpoint))
    {
        return false;
    }
    m[mountpoint] = [info](std::string proc) -> std::shared_ptr<std::istream> {
        for (auto i : info->files)
        {
            if (i.first.name == proc)
            {
                return std::make_shared<util::OMMemoryStream>(reinterpret_cast<const char *>(i.second), i.first.length);
            }
        }
        return nullptr;
    };
    vfs::info[mountpoint] = {Bundle, info};
    logger.info("bundle -> virt:{}", mountpoint);
    return false;
}
auto fsmountZipArchive(const char *src, std::size_t length, std::string mountpoint) -> bool
{
    if (mountinvaild(mountpoint))
    {
        return false;
    }

    auto pp = std::make_shared<specs::zip::OMZip>();
    pp->parse(std::make_shared<util::OMMemoryStream>(src, length));
    m[mountpoint] = [pp](std::string proc) -> std::shared_ptr<std::istream> {
        auto handle = pp->findFile(proc);
        if (!handle)
        {
            return nullptr;
        }
        return pp->read(handle);
    };
    vfs::info[mountpoint] = {Zip, pp};
    logger.info("zip -> virt:{}", mountpoint);
    return false;
}
auto compressPath(std::string vp) -> std::string
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
auto fsfetch(std::string fullPath) -> std::shared_ptr<std::istream>
{
    auto pth = compressPath(fullPath);
    for (auto p : m)
    {
        if (!pth.find(p.first))
        {
            return p.second(pth.substr(p.first.length() + 1, pth.length()));
        }
    }
    return {nullptr};
}
} // namespace openminecraft::vfs
