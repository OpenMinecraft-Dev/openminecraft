#include <iostream>
#include <stdexcept>
#include <string>

#include "fmt/base.h"
#include "fmt/color.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_saferead.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"

using namespace openminecraft::vm::elysia;
using namespace openminecraft::binary::hash;

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
        case "read"_hash: {
            std::string address;
            std::cin >> address;
            void *addr = nullptr;
            try
            {
                addr = reinterpret_cast<void *>(static_cast<uintptr_t>(std::stoll(address, nullptr, 16)));
            }
            catch (std::invalid_argument &e)
            {
                fmt::print(fmt::fg(fmt::color::alice_blue), "@unknown");
                fmt::println("");
                break;
            }
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
        }
    }

    delete wld;

    return 0;
}
