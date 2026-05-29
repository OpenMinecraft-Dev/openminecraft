#include <iostream>
#include <stdexcept>
#include <string>

#include "fmt/base.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_common.hpp"
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
        case "readmem"_hash: {
            std::string address;
            std::cin >> address;
            void *addr = nullptr;
            try
            {
                addr = reinterpret_cast<void *>(static_cast<uintptr_t>(std::stoll(address, nullptr, 16)));
            }
            catch (std::invalid_argument &e)
            {
                fmt::println("@unknown");
            }
            auto result = openminecraft::mem::safeRead<uint8_t>(addr);
            if (result.has_value())
            {
                fmt::println("@{} = 0x{:02x}", addr, result.value());
            }
            else
            {
                fmt::println("@{} = unaccessible", addr);
            }
            break;
        }
        }
    }

    delete wld;

    return 0;
}
