#include "openminecraft/specs/vfsbundle/om_vfsbundle.hpp"
#include <iostream>

#include <bitset>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef OM_PLATFORM_WINDOWS
#include "getopt.h"
#else
#include <getopt.h>
#endif

using namespace openminecraft::specs::vfsbundle;

struct file_path
{
    std::string full;
    std::string name;
};

static void appendFile(std::vector<file_path> &pth, const std::string& base, const std::string& rt)
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
        std::cout << "Usage: " << std::filesystem::path(argv[0]).filename().string() << usagetext << '\n';
        return 1;
    }

    bool isread = true;
    const auto *author = "Cyrene";
    std::string targetfile;
    std::vector<std::string> filelist = {};

    int opt = 0;
    int option_index = 0;
    const char *optstring = "h:a:t:P:cr";
    static option long_options[] = {{"read", no_argument, nullptr, 'r'},
                                    {"create", no_argument, nullptr, 'c'},
                                    {"push", optional_argument, nullptr, 'P'},
                                    {"author", optional_argument, nullptr, 'a'},
                                    {"target", optional_argument, nullptr, 't'},
                                    {"help", no_argument, nullptr, 'h'},
                                    {nullptr, 0, nullptr, 0}};
    while ((opt = getopt_long_only(argc, argv, optstring, long_options, &option_index)) != -1)
    {
        switch (opt)
        {
        case 'h':
            std::cout << "Usage: " << std::filesystem::path(argv[0]).filename().string() << usagetext << '\n';
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
            std::cerr << "Cannot read file " << targetfile << '\n';
            return 1;
        }

        OMBundle om(ins);
        std::cout << "Contents of " << targetfile << ": " << '\n';
        for (const auto &[metadata, content] : om.files)
        {
            std::cout << fmt::format("{}", metadata) << '\n';
        }

        return 0;
    }

    OMBundle om;
    if (filelist.empty())
    {
        std::cerr << "no files added!" << '\n';
        return 1;
    }

    std::vector<file_path> filesa;
    for (auto &file : filelist)
    {
        if (!std::filesystem::exists(file))
        {
            std::cerr << "file does not exist: " << file << '\n';
            continue;
        }

        appendFile(filesa, file, file);
    }

    for (auto const &l : filesa)
    {
        std::ifstream ifs(l.full, std::ios::binary);
        om.appendFile({static_cast<uint64_t>(time(nullptr)), 0, l.name, author}, ifs);
        std::cout << "file append: " << l.name << '\n';
    }

    std::ofstream ofs(targetfile, std::ios::binary);
    om.saveBundle(ofs);
    ofs.close();

    return 0;
}
