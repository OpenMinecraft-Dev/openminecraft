#include <iostream>
#include "openminecraft/specs/vfsbundle/om_vfsbundle.hpp"

#include <bitset>
#include <filesystem>
#include <fstream>

using namespace openminecraft::specs::vfsbundle;

const char *usagetext = "Usage: bundlemaker [options] <file>\nthis tool will read the target file in default\n\n  -r\treads the bundle file\n  -c\tcreate bundle file, target output file is the last arg";

struct file_path
{
    std::string full;
    std::string name;
};

static void appendFile(std::vector<file_path> &pth, std::string base, std::string rt)
{
    if (std::filesystem::is_directory(base))
    {
        for (auto const &it : std::filesystem::directory_iterator(base))
        {
            appendFile(pth, it.path().string(), rt);
        }
    }
    else
    {
        std::filesystem::path pp(base);
        pth.push_back({base, pp.lexically_relative(rt).string()});
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cout << usagetext << std::endl;
        return 1;
    }

    bool isread = true;

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            for (int ci = 1; ci < std::strlen(argv[i]); ci++)
            {
                switch (argv[i][ci])
                {
                    case 'r':
                        isread = true;
                        break;
                    case 'c':
                        isread = false;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (isread)
    {
        auto ins = std::make_shared<std::ifstream>(argv[argc - 1]);
        if (!ins->good())
        {
            std::cerr << "Cannot read file " << argv[argc - 1] << std::endl;
            return 1;
        }

        OMBundle om(ins);
        std::cout << "Contents of " << argv[argc - 1] << ": " << std::endl;
        for (const auto &[metadata, content] : om.files)
        {
            auto c = static_cast<time_t>(metadata.timestamp);
            auto tstr = ctime(&c);
            auto tstrmod = tstr;
            while (*tstrmod != '\0')
            {
                if (*tstrmod == '\n')
                {
                    *tstrmod = '\0';
                }
                tstrmod++;
            }

            std::stringstream ss;
            ss << std::setprecision(2);
            if (metadata.length < 1024)
            {
                ss << metadata.length << "B\t\t";
            }
            else if (metadata.length < 1024 * 1024)
            {
                ss << std::fixed << static_cast<double>(metadata.length) / 1024 << "kB\t";
            }
            else if (metadata.length < 1024 * 1024 * 1024)
            {
                ss << std::fixed << static_cast<double>(metadata.length) / 1024 / 1024 << "mB\t";
            }
            else if (metadata.length < 1024l * 1024 * 1024 * 1024)
            {
                ss << std::fixed << static_cast<double>(metadata.length) / 1024 / 1024 / 1024 << "gB\t";
            }
            else
            {
                ss << std::fixed << static_cast<double>(metadata.length) / 1024 / 1024 / 1024 / 1024 << "tB\t";
            }

            fmt::print("{:<10}{:<40}{:<50}{:<60}\n", metadata.owner, tstr, ss.str(), metadata.name);
        }

        return 0;
    }

    OMBundle om;
    std::vector<std::string> files;
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            files.emplace_back(argv[i]);
        }
    }

    std::string target = files[files.size() - 1];
    files.pop_back();

    if (files.empty())
    {
        std::cerr << "no files added!" << std::endl;
        return 1;
    }

    std::vector<file_path> filesa;
    for (auto &file : files)
    {
        if (!std::filesystem::exists(file))
        {
            std::cerr << "file does not exist: " << file << std::endl;
            continue;
        }

        appendFile(filesa, file, file);
    }

    for (auto const &l : filesa)
    {
        std::ifstream ifs(l.full);
        om.appendFile({static_cast<uint64_t>(time(nullptr)), 0, l.name, "Cyrene"}, ifs);
    }

    std::ofstream ofs(target);
    om.saveBundle(ofs);

    return 0;
}
