#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <vector>

#include "fmt/format.h"
#include "ftxui/component/animation.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/canvas.hpp"
#include "ftxui/dom/elements.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/vm/bytecode/om_bytecodes.hpp"
#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
#include "openminecraft/vm/elysia/om_elysia_klass.hpp"
#include "openminecraft/vm/elysia/om_elysia_threadmodel.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"

using namespace ftxui;
using namespace openminecraft::vm::elysia;
using openminecraft::mem::castorice::printres;

Element buildMemComp()
{
    std::vector<std::vector<Element>> memcomps;
    printres([&](std::string a, std::string b) {
        memcomps.push_back(std::vector{text(a) | flex, text(b) | flex | color(Color::Green)});
    });
    return window(text("Memory"), gridbox(memcomps));
}

std::string toDataSize(uint64_t l)
{
    if (l < 1024)
    {
        return fmt::format("{} B", l);
    }
    else if (l < 1024 * 1024)
    {
        return fmt::format("{:.{}f} KB", (double)l / 1024, 2);
    }
    else if (l < 1024 * 1024 * 1024)
    {
        return fmt::format("{:.{}f} MB", (double)l / 1024 / 1024, 2);
    }
    else if (l < 1024ll * 1024 * 1024 * 1024)
    {
        return fmt::format("{:.{}f} GB", (double)l / 1024 / 1024 / 1024, 2);
    }
    else
    {
        return fmt::format("{:.{}f} TB", (double)l / 1024 / 1024 / 1024 / 1024, 2);
    }
}

Element buildElysiaHeapComp(OMElysiaHeap &heap)
{
    auto c = Canvas(60, 60);

    std::vector<std::pair<uint64_t, bool>> lengths;
    void *current = heap.base();
    heap.iterBlocks([&](OMElysiaHeapBlock *k) {
        if (current != k->block)
        {
            lengths.push_back(
                std::make_pair(reinterpret_cast<uintptr_t>(k->block) - reinterpret_cast<uintptr_t>(current), true));
        }

        lengths.push_back(
            std::make_pair(reinterpret_cast<uintptr_t>(k->blockEnd) - reinterpret_cast<uintptr_t>(k->block), false));

        current = k->blockEnd;
    });

    uint64_t length = 0;
    uint64_t lengthUsed = 0;
    for (auto &l : lengths)
    {
        length += l.first;
        if (l.second)
        {
            lengthUsed += l.first;
        }
    }

    for (auto &l : lengths)
    {
        l.first = std::round(static_cast<double>(l.first) / length * 3600);
    }

    uint64_t index = 0;
    while (!lengths.empty())
    {
        if (lengths[0].first == 0)
        {
            lengths.erase(lengths.begin());
            continue;
        }

        c.DrawPoint(index % 60, index / 60, lengths[0].second, Color::Green);

        index++;
        lengths[0].first--;
    }

    return vbox({hbox({text(toDataSize(lengthUsed)) | color(Color::Green), separatorEmpty() | flex,
                       text(fmt::format("{}", heap.base())) | color(Color::Blue), separatorEmpty() | flex,
                       text(toDataSize(length)) | color(Color::GrayLight)}),
                 canvas(c)});
}

Element buildElysiaHeapComp2(OMElysiaVirtualWorld *world)
{
    return window(text("Elysia Heap"), hbox({window(text("Metaspace"), buildElysiaHeapComp(world->metaspaceHeap)),
                                             window(text("Main"), buildElysiaHeapComp(world->mainHeap))}));
}

Element buildElysiaThreadAssembly(OMElysiaThread *thread)
{
    std::vector<std::vector<Element>> codelines;
    auto mm = thread->zero.frame->method;
    codelines.push_back({text(fmt::format("{}", (void *)thread->zero.pc))});
    codelines.push_back({text(fmt::format("{}", (char *)mm->klass->name)) | color(Color::GrayDark)});
    codelines.push_back({text(fmt::format("{}{}", mm->name, mm->descriptor)) | color(Color::GrayDark)});

    uint8_t *code = reinterpret_cast<uint8_t *>(thread->zero.pc);
    uint16_t s;
    int16_t ss;
    uint8_t j;
    while (code < thread->zero.frame->method->code + thread->zero.frame->method->codeLength)
    {
#define OpArgs16(name)                                                                                                 \
    ss = static_cast<int16_t>(code[1]) << 8 | code[2];                                                                 \
    codelines.push_back(                                                                                               \
        {text(fmt::format("{:02x} {:02x} {:02x}", *code, code[1], code[2])) | color(Color::Green) | flex,              \
         text(fmt::format("{} {}", name, ss)) | color(Color::GrayLight) | flex});                                      \
    code += 3;
#define OpArgu16(name)                                                                                                 \
    s = static_cast<uint16_t>(code[1]) << 8 | code[2];                                                                 \
    codelines.push_back(                                                                                               \
        {text(fmt::format("{:02x} {:02x} {:02x}", *code, code[1], code[2])) | color(Color::Green) | flex,              \
         text(fmt::format("{} {}", name, s)) | color(Color::GrayLight) | flex});                                       \
    code += 3;
#define OpArgu8(name)                                                                                                  \
    j = code[1];                                                                                                       \
    codelines.push_back({text(fmt::format("{:02x} {:02x}", *code, code[1])) | color(Color::Green) | flex,              \
                         text(fmt::format("{} {}", name, (int)j)) | color(Color::GrayLight) | flex});                  \
    code += 2;
#define OpArgv(name)                                                                                                   \
    codelines.push_back({text(fmt::format("{:02x}", *code)) | color(Color::Green) | flex,                              \
                         text(name) | color(Color::GrayLight) | flex});                                                \
    ++code;
        switch (*code)
        {
        case op_nop:
            OpArgv("nop");
            break;
        case op_iconst_i(-1):
            OpArgv("iconst_n1");
            break;
        case op_iconst_i(0):
            OpArgv("iconst_0");
            break;
        case op_iconst_i(1):
            OpArgv("iconst_1");
            break;
        case op_iconst_i(2):
            OpArgv("iconst_2");
            break;
        case op_iconst_i(3):
            OpArgv("iconst_3");
            break;
        case op_iconst_i(4):
            OpArgv("iconst_4");
            break;
        case op_iconst_i(5):
            OpArgv("iconst_5");
            break;
        case op_lconst_l(0):
            OpArgv("lconst_0");
            break;
        case op_lconst_l(1):
            OpArgv("lconst_1");
            break;
        case op_fconst_f(0):
            OpArgv("fconst_0");
            break;
        case op_fconst_f(1):
            OpArgv("fconst_1");
            break;
        case op_fconst_f(2):
            OpArgv("fconst_2");
            break;
        case op_dconst_d(0):
            OpArgv("dconst_0");
            break;
        case op_dconst_d(1):
            OpArgv("dconst_1");
            break;
        case op_bipush:
            OpArgu8("bipush");
            break;
        case op_sipush:
            OpArgs16("sipush");
            break;
        case op_iload_n(0):
            OpArgv("iload_0");
            break;
        case op_iload_n(1):
            OpArgv("iload_1");
            break;
        case op_iload_n(2):
            OpArgv("iload_2");
            break;
        case op_iload_n(3):
            OpArgv("iload_3");
            break;
        case op_lload_n(0):
            OpArgv("lload_0");
            break;
        case op_lload_n(1):
            OpArgv("lload_1");
            break;
        case op_lload_n(2):
            OpArgv("lload_2");
            break;
        case op_lload_n(3):
            OpArgv("lload_3");
            break;
        case op_istore_n(1):
            OpArgv("istore_1");
            break;
        case op_ldc:
            OpArgu8("ldc");
            break;
        case op_getstatic:
            OpArgu16("getstatic");
            break;
        case op_invokevirtual:
            OpArgu16("invokevirtual");
            break;
        case op_invokespecial:
            OpArgu16("invokespecial");
            break;
        case op_new:
            OpArgu16("new");
            break;
        case op_dup:
            OpArgv("dup");
            break;
        case op_goto:
            OpArgs16("goto");
            break;
        default:
            codelines.push_back({text(fmt::format("{:02x}", *code)) | color(Color::Green) | flex,
                                 text("<unknown operand>") | color(Color::GrayDark) | flex});
            ++code;
            break;
        }
    }

    return gridbox(codelines);
}

Element buildElysiaThreadCode(OMElysiaThread *thread)
{
    if (thread->zero.pc && thread->zero.frame)
    {
        auto c = reinterpret_cast<uint8_t *>(thread->zero.pc);
        auto tt = text(fmt::format("{}", thread->zero.pc));
        return buildElysiaThreadAssembly(thread);
    }
    else
    {
        return text(fmt::format("(No code)"));
    }
}

Element buildElysiaThreadStack(OMElysiaThread *thread)
{
    std::vector<std::vector<Element>> stk;
    void **pp = reinterpret_cast<void **>(thread->zero.stackPointer);
    while (pp < thread->stackStart)
    {
        stk.push_back({separatorEmpty() | flex, text(fmt::format("{}", fmt::ptr(pp))), separatorEmpty() | flex,
                       text(fmt::format("{:0" + fmt::format("{}", sizeof(void *) * 2) + "x}", (uintptr_t)*pp)) |
                           color(Color::Blue)});
        ++pp;
    }
    return gridbox(stk) | flex;
}

Element buildElysiaThread(OMElysiaThread *thread)
{
    return hbox(
        {window(text("Code"), buildElysiaThreadCode(thread)), window(text("Stack"), buildElysiaThreadStack(thread))});
}

Element buildElysiaThreadstate()
{
    std::lock_guard lg(mapMutex);
    if (threadMap.empty())
    {
        return text(fmt::format("No threads"));
    }
    std::vector<Element> elem;
    for (auto &t : threadMap)
    {
        elem.push_back(window(text(fmt::format("Thread {}", reinterpret_cast<const void *>(&t.first))),
                              buildElysiaThread(t.second)));
    }

    return window(text("Elysia Threads"), vbox(elem));
}

int main(int argc, const char *argv[])
{
    auto wld = new OMElysiaVirtualWorld;

    auto container = Container::Horizontal({});
    auto renderer = Renderer(container, [&] {
        animation::RequestAnimationFrame();

        return vbox(
            {buildMemComp(), window(text("ElysiaVM"), vbox({buildElysiaHeapComp2(wld), buildElysiaThreadstate()}))});
    });

    auto screen = ScreenInteractive::Fullscreen();
    screen.Loop(renderer);

    delete wld;

    return 0;
}
