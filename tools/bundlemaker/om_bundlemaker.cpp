#include <iostream>
#include "openminecraft/specs/vfsbundle/om_vfsbundle.hpp"

#include <bitset>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "getopt.h"

using namespace openminecraft::specs::vfsbundle;

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

const char *usagetext = " [options] <file>\n"
"this tool will read the target file in default\n"
"\n"
"  -r / --read\tread the bundle file\n"
"  -c / --create\tcreate bundle file\n"
"  -P / --push\tonly works in create mode, attach file to new archives\n"
"  -a / --author\tonly works in create mode, set the name of the author\n"
"  -t / --target\tset the target input/output file\n"
"  -h / --help\tshow this message\n"
"\n"
"For example: \n"
"  -a Cyrene -c -t out.bundle -P hello_world.txt\n"
"    create a bundle out.bundle with file hello_world.txt (author is Cyrene)\n"
"  -r -t out.bundle\n"
"    read the out.bundle";

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        std::cout << "Usage: " << std::filesystem::path(argv[0]).filename().string() << usagetext << std::endl;
        return 1;
    }

    bool isread = true;
    auto author = "Cyrene";
    std::string targetfile = "";
    std::vector<std::string> filelist = {};

    int opt;
    int option_index = 0;
    const char *optstring = "h:a:t:P:c:r";
    static option long_options[] = {
        {"help", no_argument, nullptr, 'h'},
        {"read", no_argument, nullptr, 'r'},
        {"create", no_argument, nullptr, 'c'},
        {"push", optional_argument, nullptr, 'P'},
        {"author", optional_argument, nullptr, 'a'},
        {"target", optional_argument, nullptr, 't'},
        {nullptr, 0, nullptr, 0}
    };
    while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
        switch (opt)
        {
        case 'h':
            std::cout << "Usage: " << std::filesystem::path(argv[0]).filename().string() << usagetext << std::endl;
            return 0;
        case 'r':
            isread = true;
            break;
        case 'c':
            isread = false;
            break;
        case 'a':
            author = optarg;
            break;
        case 't':
            targetfile = optarg;
            break;
        case 'P':
            filelist.emplace_back(optarg);
            break;
        default:
            break;
        }
    }

    if (isread)
    {
        auto ins = std::make_shared<std::ifstream>(targetfile);
        if (!ins->good())
        {
            std::cerr << "Cannot read file " << targetfile << std::endl;
            return 1;
        }

        OMBundle om(ins);
        std::cout << "Contents of " << targetfile << ": " << std::endl;
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
    if (filelist.empty())
    {
        std::cerr << "no files added!" << std::endl;
        return 1;
    }

    std::vector<file_path> filesa;
    for (auto &file : filelist)
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
        om.appendFile({static_cast<uint64_t>(time(nullptr)), 0, l.name, author}, ifs);
    }

    std::ofstream ofs(targetfile);
    om.saveBundle(ofs);

    return 0;
}
