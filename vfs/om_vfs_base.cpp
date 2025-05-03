#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <filesystem>
#include <fstream>
#include <ios>
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
    else
    {
        return false;
    }
    return false;
}
bool fsmountBundle(BundleInfo info, std::string mountpoint)
{
    if (mountinvaild(mountpoint))
    {
        return false;
    }
    m[mountpoint] = [info](std::string proc) -> std::shared_ptr<std::istream> {
        if (!proc.empty() && proc[0] == '/')
        {
            proc = proc.substr(1);
        }
        std::istringstream str(std::string((char *)info.p, info.length));
        auto rdlen = [&str] {
            uint8_t n0, n1, n2, n3, n4, n5, n6, n7;
            str.read((char *)&n0, 1);
            str.read((char *)&n1, 1);
            str.read((char *)&n2, 1);
            str.read((char *)&n3, 1);
            str.read((char *)&n4, 1);
            str.read((char *)&n5, 1);
            str.read((char *)&n6, 1);
            str.read((char *)&n7, 1);
            auto nl0 = (uint64_t)n0;
            auto nl1 = (uint64_t)n1;
            auto nl2 = (uint64_t)n2;
            auto nl3 = (uint64_t)n3;
            auto nl4 = (uint64_t)n4;
            auto nl5 = (uint64_t)n5;
            auto nl6 = (uint64_t)n6;
            auto nl7 = (uint64_t)n7;
            return nl0 | (nl1 << 8) | (nl2 << 16) | (nl3 << 24) | (nl4 << 32) | (nl5 << 40) | (nl6 << 48) | (nl7 << 56);
        };
        while (str.good())
        {
            auto l = rdlen();
            if (!str.good())
            {
                break;
            }
            auto name = new char[l + 1];
            name[l] = '\0';
            str.read(name, l);
            auto dl = rdlen();
            if (name != proc)
            {
                str.seekg(str.tellg() + (std::streamoff)dl);
                delete[] name;
            }
            else
            {
                auto data = new char[dl];
                str.read(data, dl);
                delete[] name;
                return std::make_shared<std::istringstream>(std::string(data, dl));
            }
        }
        return nullptr;
    };
    openminecraft::vfs::info[mountpoint] = {Bundle, info};
    logger.info("bundle:{}+{} -> virt:{}", info.p, info.length, mountpoint);
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
            continue;
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
            return p.second(pth.substr(p.first.length(), pth.length()));
        }
    }
    return std::shared_ptr<std::istream>(nullptr);
}
} // namespace openminecraft::vfs