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
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"

using namespace openminecraft::vm::elysia;
using namespace openminecraft::binary::hash;

template <typename T> void printOopFieldContent(T *t)
{
    if constexpr (std::is_same_v<T, jbyte>)
    {
        fmt::print(fmt::fg(fmt::color::green), "{}", static_cast<jint>(*t));
    }
    else if constexpr (std::is_same_v<T, jboolean>)
    {
        fmt::print(fmt::fg(fmt::color::green), "{}", *t ? "true" : "false");
    }
    else
    {
        fmt::print(fmt::fg(fmt::color::green), "{}", *t);
    }
}

void printOopFields(OMElysiaVirtualWorld *world, OMElysiaOop *oop, OMElysiaInstanceKlass *klass)
{
    for (int i = 0; i < klass->fieldCount; i++)
    {
        auto &field = klass->fields[i];
        if (field.isStatic())
        {
            continue;
        }
        fmt::print(fmt::fg(fmt::color::alice_blue), "@+0x{:x} ", field.offset);
        fmt::print(fmt::fg(fmt::color::white_smoke), "{}", field.name);
        fmt::print(":");
        fmt::print(fmt::fg(fmt::color::slate_blue), "{}", field.desc);
        fmt::print(" - ");

        switch (*field.desc)
        {
#define CASEP(n, type)                                                                                                 \
    case n:                                                                                                            \
        printOopFieldContent(reinterpret_cast<type *>(world->oopManager->oopAccessField(oop, field.offset)));          \
        break;

            CASEP('Z', jboolean);
            CASEP('B', jbyte);
            CASEP('C', jchar);
            CASEP('S', jshort);
            CASEP('F', jfloat);
            CASEP('I', jint);
            CASEP('J', jlong);
            CASEP('D', jdouble);

        case 'L':
        case '[': {
            if (world->mainHeap.enablePtrCompress())
            {
                auto ptrr = *reinterpret_cast<uint32_t *>(world->oopManager->oopAccessField(oop, field.offset));
                fmt::print(fmt::fg(fmt::color::alice_blue), "@{} ({:08x})", world->mainHeap.decompress(ptrr), ptrr);
            }
            else
            {
                printOopFieldContent(reinterpret_cast<void **>(world->oopManager->oopAccessField(oop, field.offset)));
            }
            break;
        }
        default:
            break;
        }

        fmt::println("");
    }
}

void readMem(OMElysiaVirtualWorld *world)
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
    case "oop"_hash: {
        if (!world->mainHeap.valid(addr))
        {
            fmt::print(fmt::fg(fmt::color::red), "invalid oop");
            fmt::println("");
            break;
        }
        auto klass = world->oopManager->oopGetKlass(reinterpret_cast<OMElysiaOop *>(addr));
        fmt::print(fmt::fg(fmt::color::alice_blue), "@{} ", addr);
        if (world->metaspaceHeap.valid(klass))
        {
            fmt::print("is an oop of klass {}", klass->name);
            fmt::print(fmt::fg(fmt::color::yellow_green), " ({})", klass->isArray() ? "array" : "instance");
            fmt::println("");

            if (klass->isInstance())
            {
                auto instanceKlass = klass->toInstance();
                fmt::print("object length ");
                fmt::print(fmt::fg(fmt::color::alice_blue), "{}", instanceKlass->length);
                fmt::println("");
                fmt::println("fields: ");
                printOopFields(world, reinterpret_cast<OMElysiaOop *>(addr), instanceKlass);
            }
            else
            {
                auto arrl = world->oopManager->arrLength(reinterpret_cast<OMElysiaOop *>(addr));
                fmt::print("Array of length ");
                fmt::print(fmt::fg(fmt::color::alice_blue), "{}", arrl);
                fmt::println("");

		for (int i = 0; i < std::min(arrl, 8); i++)
		{
		    fmt::print(fmt::fg(fmt::color::yellow), "[{}] ", i);
		    fmt::print("= ");
		    printOopFieldContent(&world->oopManager->arrAccess<jchar>(reinterpret_cast<OMElysiaOop *>(addr))[i]);
		    fmt::println("");
		}

		if (arrl > 8)
		{
		    fmt::println("...");
		}
            }
        }
        else
        {
            fmt::println("is invalid");
        }
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
            readMem(wld);
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
