#include <array>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fmt/base.h"
#include "fmt/color.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_saferead.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"

using namespace openminecraft::vm::elysia;
using namespace openminecraft::binary::hash;

void readMem()
{
    std::string type, address;
    std::cin >> type >> address;
    void *addr = nullptr;
    try
    {
        addr = reinterpret_cast<void *>(static_cast<uintptr_t>(std::stoll(address, nullptr, 16)));
    }
    catch (std::invalid_argument &e)
    {
        fmt::print(fmt::fg(fmt::color::alice_blue), "@unknown");
        fmt::println("");
        return;
    }

    switch (hash_compile_time(type.c_str()))
    {
    case "norm"_hash: {
#define readT(k, type, n, tt)                                                                                          \
    auto k = openminecraft::mem::safeRead<type>(addr);                                                                 \
    fmt::print(fmt::fg(fmt::color::alice_blue), "@({}){}", n, addr);                                                   \
    if (k.has_value())                                                                                                 \
    {                                                                                                                  \
        fmt::println(" = 0x{:0" + fmt::format("{}", sizeof(type) * 2) + "x}", static_cast<tt>(k.value()));             \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        fmt::println(" = unaccessible");                                                                               \
    }
        readT(u8v, int8_t, "u8", uint8_t);
        readT(u16v, int16_t, "u16", uint16_t);
        readT(u32v, int32_t, "u32", uint32_t);
        readT(u64v, int64_t, "u64", uint64_t);
        break;
    }
    case "dump"_hash: {
        uint64_t bytes;
        std::cin >> bytes;

        std::vector<std::array<std::optional<uint8_t>, 16>> memdmps;
        for (uint64_t i = 0; i < bytes; i++)
        {
            auto bb = static_cast<std::optional<uint8_t>>(
                openminecraft::mem::safeRead<int8_t>(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(addr) + i)));
            if (memdmps.size() <= (i / 16))
            {
                memdmps.push_back({});
            }

            memdmps[i / 16][i % 16] = bb;
        }

        for (auto &memline : memdmps)
        {
            fmt::print(fmt::fg(fmt::color::alice_blue), "@{}\t", addr);
            for (auto &v : memline)
            {
                if (v.has_value())
                {
                    fmt::print(fmt::fg(v.value() ? fmt::color::white_smoke : fmt::color::dark_gray), "{:02x}\t",
                               v.value());
                }
                else
                {
                    fmt::print(fmt::fg(fmt::color::red), "??\t");
                }
            }
            fmt::print("\t");
            for (auto &v : memline)
            {
                if (v.has_value())
                {
                    if (v.value() >= 0x20 && v.value() < 0x7f)
                    {
                        fmt::print(fmt::fg(fmt::color::white_smoke), "{}", static_cast<char>(v.value()));
                    }
                    else
                    {
                        fmt::print(fmt::fg(v.value() ? fmt::color::white_smoke : fmt::color::dark_gray), ".");
                    }
                }
                else
                {
                    fmt::print(fmt::fg(fmt::color::red), ".");
                }
            }
            fmt::println("");
            addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(addr) + 16);
        }

        break;
    }
    default:
        fmt::print(fmt::fg(fmt::color::red), "Unknown command");
        fmt::println("");
        break;
    }
}

int main(int argc, const char *argv[])
{
    openminecraft::log::multithread::registerCurrentThreadName("Bootstrap");
    auto wld = new OMElysiaVirtualWorld;

    while (true)
    {
        std::string command;
        std::cin >> command;

        switch (hash_compile_time(command.c_str()))
        {
        case "exit"_hash: {
            std::exit(0);
        }
        case "read"_hash: {
            readMem();
            break;
        }
        default: {
            fmt::print(fmt::fg(fmt::color::red), "Unknown command");
            fmt::println("");
            break;
        }
        }
    }

    delete wld;

    return 0;
}
