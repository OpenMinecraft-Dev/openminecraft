#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "fmt/base.h"
#include "fmt/color.h"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/log/om_log_threadname.hpp"
#include "openminecraft/mem/om_mem_saferead.hpp"
#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_oopmanager.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_types.hpp"
#include "openminecraft/vm/elysia/om_elysium.hpp"

using namespace std::chrono_literals;
using namespace openminecraft::vm::elysia;
using namespace openminecraft::binary::hash;

constexpr auto addrColor = fmt::color::slate_blue;
constexpr auto hintColor = fmt::color::gray;
constexpr auto valueColor = fmt::color::green;
constexpr auto errorColor = fmt::color::red;

template <typename T> void printOopFieldContent(T *t)
{
    if constexpr (std::is_same_v<T, jbyte>)
    {
        fmt::print(fmt::fg(valueColor), "{:02x}", static_cast<jint>(*t));
    }
    else if constexpr (std::is_same_v<T, jboolean>)
    {
        fmt::print(fmt::fg(valueColor), "{}", *t ? "true" : "false");
    }
    else if constexpr (std::is_same_v<T, jchar>)
    {
        fmt::print(fmt::fg(valueColor), "{} ({})", *t, static_cast<char>(*t));
    }
    else if constexpr (std::is_same_v<T, jshort>)
    {
        fmt::print(fmt::fg(valueColor), "{} ({:04x})", *t, *t);
    }
    else if constexpr (std::is_same_v<T, jint>)
    {
        fmt::print(fmt::fg(valueColor), "{} ({:08x})", *t, *t);
    }
    else if constexpr (std::is_same_v<T, jlong>)
    {
        fmt::print(fmt::fg(valueColor), "{} ({:016x})", *t, *t);
    }
    else
    {
        fmt::print(fmt::fg(valueColor), "{}", *t);
    }
}

void printOopFields(OMElysium *elysium, OMElysiaOop *oop, OMElysiaInstanceKlass *klass)
{
    for (int i = 0; i < klass->fieldCount; i++)
    {
        auto &field = klass->fields[i];
        if (field.isStatic())
        {
            continue;
        }
        fmt::print(fmt::fg(addrColor), "@+0x{:x} ", field.offset);
        fmt::print("{}", field.name);
        fmt::print(":");
        fmt::print(fmt::fg(hintColor), "{}", field.desc);
        fmt::print(" - ");

        switch (*field.desc)
        {
#define CASEP(n, type)                                                                                                 \
    case n:                                                                                                            \
        printOopFieldContent(reinterpret_cast<type *>(elysium->oopManager->oopAccessField(oop, field.offset)));        \
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
            if (elysium->mainHeap.enablePtrCompress())
            {
                auto ptrr = *reinterpret_cast<uint32_t *>(elysium->oopManager->oopAccessField(oop, field.offset));
                fmt::print(fmt::fg(addrColor), "@{} ({:08x})", elysium->mainHeap.decompress(ptrr), ptrr);
            }
            else
            {
                printOopFieldContent(reinterpret_cast<void **>(elysium->oopManager->oopAccessField(oop, field.offset)));
            }
            break;
        }
        default:
            break;
        }

        fmt::println("");
    }
}

void printOop(OMElysium *world, void *addr, bool simple = false)
{
    if (!world->mainHeap.valid(addr))
    {
        fmt::print(fmt::fg(errorColor), "invalid oop");
        fmt::println("");
        return;
    }
    auto klass = world->oopManager->oopGetKlass(reinterpret_cast<OMElysiaOop *>(addr));
    fmt::print(fmt::fg(addrColor), "@{} ", addr);
    if (world->metaspaceHeap.valid(klass))
    {
        fmt::print("is an oop of klass {}", klass->name);
        fmt::print(fmt::fg(valueColor), " ({})", klass->isArray() ? "array" : "instance");
        fmt::println("");

        if (simple)
        {
            return;
        }

        if (klass->isInstance())
        {
            auto instanceKlass = klass->toInstance();
            fmt::print("object length ");
            fmt::print(fmt::fg(addrColor), "{}", instanceKlass->length);
            fmt::println("");
            fmt::println("fields: ");
            printOopFields(world, reinterpret_cast<OMElysiaOop *>(addr), instanceKlass);
        }
        else
        {
            auto arrl = world->oopManager->arrLength(reinterpret_cast<OMElysiaOop *>(addr));
            fmt::print("Array of length ");
            fmt::print(fmt::fg(addrColor), "{}", arrl);
            fmt::println("");

            for (int i = 0; i < std::min(arrl, 8); i++)
            {
                fmt::print(fmt::fg(hintColor), "[{}] ", i);
                fmt::print("= ");
                switch (klass->name[1])
                {
#define CASEAP(n, type)                                                                                                \
    case n:                                                                                                            \
        printOopFieldContent(&world->oopManager->arrAccess<type>(reinterpret_cast<OMElysiaOop *>(addr))[i]);           \
        break;
                    CASEAP('Z', jboolean);
                    CASEAP('B', jbyte);
                    CASEAP('S', jshort);
                    CASEAP('C', jchar);
                    CASEAP('I', jint);
                    CASEAP('F', jfloat);
                    CASEAP('J', jlong);
                    CASEAP('D', jdouble);
                case 'L':
                case '[': {
                    if (world->mainHeap.enablePtrCompress())
                    {
                        auto ptrr = world->oopManager->arrAccess<uint32_t>(reinterpret_cast<OMElysiaOop *>(addr))[i];
                        fmt::print(fmt::fg(addrColor), "@{} ({:08x})", world->mainHeap.decompress(ptrr), ptrr);
                    }
                    else
                    {
                        printOopFieldContent(
                            &world->oopManager->arrAccess<void *>(reinterpret_cast<OMElysiaOop *>(addr))[i]);
                    }
                    break;
                }
                default:
                    fmt::print(fmt::fg(errorColor), "invalid");
                    break;
                }
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
}

void readMem(OMElysium *elysium)
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
        fmt::print(fmt::fg(addrColor), "@unknown");
        fmt::println("");
        return;
    }

    switch (hash_compile_time(type.c_str()))
    {
    case "norm"_hash: {
#define readT(k, type, n, tt)                                                                                          \
    auto k = openminecraft::mem::safeRead<type>(addr);                                                                 \
    fmt::print(fmt::fg(addrColor), "@({}){}", n, addr);                                                                \
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
        printOop(elysium, addr);
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
            fmt::print(fmt::fg(addrColor), "@{}\t", addr);
            for (auto &v : memline)
            {
                if (v.has_value())
                {
                    if (v.value())
                    {
                        fmt::print("{:02x}\t", v.value());
                    }
                    else
                    {
                        fmt::print(fmt::fg(hintColor), "{:02x}\t", v.value());
                    }
                }
                else
                {
                    fmt::print(fmt::fg(errorColor), "??\t");
                }
            }
            fmt::print("\t");
            for (auto &v : memline)
            {
                if (v.has_value())
                {
                    if (v.value() >= 0x20 && v.value() < 0x7f)
                    {
                        fmt::print("{}", static_cast<char>(v.value()));
                    }
                    else
                    {
                        if (v.value())
                        {
                            fmt::print(".");
                        }
                        else
                        {
                            fmt::print(fmt::fg(hintColor), ".");
                        }
                    }
                }
                else
                {
                    fmt::print(fmt::fg(errorColor), ".");
                }
            }
            fmt::println("");
            addr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(addr) + 16);
        }

        break;
    }
    default:
        fmt::print(fmt::fg(errorColor), "Unknown command");
        fmt::println("");
        break;
    }
}

void search(OMElysium *elysium)
{
    uint64_t objs = 0;
    auto base = reinterpret_cast<OMElysiaOop *>(elysium->mainHeap.rawHeap.block);

    std::lock_guard guard(elysium->mainHeap.blockMutex);

begin:
    OMElysiaKlass *klass = elysium->oopManager->oopGetKlass(base);
    if (elysium->metaspaceHeap.valid(klass))
    {
        goto print;
    }
    base = reinterpret_cast<OMElysiaOop *>(reinterpret_cast<uintptr_t>(base) + 8);
    goto begin;

print:
    objs++;
    printOop(elysium, base, true);
    base = reinterpret_cast<OMElysiaOop *>(reinterpret_cast<uintptr_t>(base) + elysium->oopManager->oopLength(base));

    if (base >= elysium->mainHeap.rawHeap.heapTop)
    {
        fmt::print(fmt::fg(addrColor), "{}", objs);
        fmt::println(" objects");
        return;
    }

    auto node = elysium->mainHeap.emptyBlocks;
    while (node)
    {
        if (base >= node->block && base < node->blockEnd)
        {
            fmt::print(fmt::fg(hintColor), "{} ~ {} (free)", node->block, node->blockEnd);
            fmt::println("");
            base = reinterpret_cast<OMElysiaOop *>(node->blockEnd);
            if (!elysium->mainHeap.valid(base))
            {
                fmt::print(fmt::fg(addrColor), "{}", objs);
                fmt::println(" objects");
                return;
            }
            break;
        }
        node = node->next;
    }

    goto begin;
}

void printStackStatus()
{
    for (auto &th : threadMap)
    {
        fmt::println("status for thread {}", reinterpret_cast<const void *>(&th.first));
        fmt::print("stack base at ");
        fmt::print(fmt::fg(addrColor), "@0x{:x}", th.second->stackStart);
        fmt::println("");
        fmt::print("stack top at ");
        fmt::print(fmt::fg(addrColor), "@0x{:x}", th.second->stackEnd);
        fmt::println("");

        switch (th.second->state)
        {
#define cse(name)                                                                                                      \
    case name:                                                                                                         \
        fmt::print(fmt::fg(hintColor), #name);                                                                         \
        fmt::println("");                                                                                              \
        break;
            cse(Halt);
            cse(Initialized);
            cse(InsideVM);
            cse(InsideNative);
            cse(InsideJava);
            cse(Suspend);
        }
        fmt::print(fmt::fg(addrColor), "stack trace:");
        fmt::println("");
        if (th.second->zero.frame)
        {
            auto frm = th.second->zero.frame;
            auto ptr = th.second->zero.pc;
            int i = 0;
            while (frm)
            {
                if (frm->method->isNative())
                {
                    fmt::print(fmt::fg(hintColor), "Native");
                }
                fmt::print(fmt::fg(frm->method->isNative() ? valueColor : addrColor), "\t#{} {}.{}{} + {}", i,
                           frm->method->klass->name, frm->method->name, frm->method->descriptor,
                           reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(frm->method->code));
                ptr = frm->returnAddr;
                frm = frm->caller;
                ++i;

                fmt::println("");
            }
        }
        else
        {
            fmt::print(fmt::fg(hintColor), "\t(no frames)");
            fmt::println("");
        }

        fmt::println("---------------------------------------------");
    }
}

void printElysium(OMElysium *elysium)
{
    std::string type;
    std::cin >> type;

    switch (hash_compile_time(type.c_str()))
    {
    case "stack"_hash:
        printStackStatus();
        break;
    case "search"_hash:
        search(elysium);
        break;
    case "read"_hash: {
        readMem(elysium);
        break;
    }
    default:
        fmt::print(fmt::fg(errorColor), "invalid type");
        fmt::println("");
        break;
    }
}

int main(int argc, const char *argv[])
{
    openminecraft::log::multithread::registerCurrentThreadName("Bootstrap");
    auto elysium = new OMElysium;

    while (true)
    {
        std::string command;
        std::cin >> command;

        switch (hash_compile_time(command.c_str()))
        {
        case "exit"_hash: {
            int i = 0;
            std::cin >> i;
            std::exit(i);
        }
        case "elysium"_hash: {
            printElysium(elysium);
            break;
        }
        case "heaptest"_hash: {
            auto heap = new OMElysiaHeap("external_test", 1024 * 1024, 0.5);
            std::mutex test;
            std::unordered_map<std::thread::id, int> thrs;
            for (int i = 0; i < 36; i++)
            {
                auto thr = new std::thread([&]() {
                    while (true)
                    {
                        auto l = (uint64_t *)heap->allocate(8);
                        heap->deallocate(l);
                        {
                            std::lock_guard guard(test);
                            thrs[std::this_thread::get_id()]++;
                        }
                    }
                });
            }

            while (true)
            {
                std::this_thread::sleep_for(50ms);
                {
                    std::lock_guard guard(test);
                    for (auto &t : thrs)
                    {
                        std::cout << std::hex << t.first << ": " << std::dec << t.second << "times" << std::endl;
                    }
                    std::cout << "----------------" << std::endl;
                }
            }

            break;
        }
        default: {
            fmt::print(fmt::fg(errorColor), "Unknown command");
            fmt::println("");
            break;
        }
        }
    }

    delete elysium;

    return 0;
}
