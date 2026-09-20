#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/specs/zip/om_zip.hpp"
#include "openminecraft/util/om_util_memstream.hpp"
#include "openminecraft/vfs/om_vfs_bundlefs.hpp"
#include "openminecraft/vfs/om_vfs_realfs.hpp"
#include "openminecraft/vfs/om_vfs_zipfs.hpp"
#include <cstddef>
#include <iostream>
#include <istream>
#include <memory>
#include <string>
#include <unordered_map>

namespace openminecraft::vfs
{
std::unordered_map<std::string, std::shared_ptr<OMFsProvider>> fshandlers;
auto mountinvaild(std::string mp) -> bool
{
    return mp.empty() || mp[0] != '/' || mp == "/" || mp[mp.length() - 1] == '/';
}
auto fsumount(std::string mountpoint) -> bool
{
    if (fshandlers.count(mountpoint))
    {
        fshandlers.erase(mountpoint);
        return true;
    }

    return false;
}

static auto fsmount(std::shared_ptr<OMFsProvider> p, std::string mountpoint) -> bool
{
    if (mountinvaild(mountpoint))
    {
        return false;
    }
    fshandlers[mountpoint] = p;
    return true;
}

auto fsmountReal(std::string root, std::string mountpoint) -> bool
{
    return fsmount(std::make_shared<OMFsProviderReal>(root), mountpoint);
}
auto fsmountBundle(std::shared_ptr<specs::vfsbundle::OMBundle> info, std::string mountpoint) -> bool
{
    return fsmount(std::make_shared<OMFsProviderBundle>(info), mountpoint);
}
auto fsmountZipArchive(const char *src, std::size_t length, std::string mountpoint) -> bool
{
    return fsmountZipArchive(std::make_shared<util::OMMemoryStream>(src, length), mountpoint);
}
auto fsmountZipArchive(std::shared_ptr<std::istream> istr, std::string mountpoint) -> bool
{
    auto pp = std::make_shared<specs::zip::OMZip>();
    pp->parse(istr);
    return fsmount(std::make_shared<OMFsProviderZip>(pp), mountpoint);
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
    for (auto p : fshandlers)
    {
        if (!pth.find(p.first))
        {
            return p.second->read(pth.substr(p.first.length() + 1, pth.length()));
        }
    }
    return nullptr;
}
} // namespace openminecraft::vfs
