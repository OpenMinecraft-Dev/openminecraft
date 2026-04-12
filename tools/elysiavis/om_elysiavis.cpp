#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
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
    return gridbox(memcomps);
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

    std::vector<Element> codetop;
    codetop.push_back(
        text(fmt::format("{} + {}", (void *)mm->code, static_cast<uintptr_t>(thread->zero.pc - mm->code))));
    codetop.push_back(text(fmt::format("{}", (char *)mm->klass->name)) |
                      color(Color::GrayDark));
    codetop.push_back(text(fmt::format("{}{}", mm->name, mm->descriptor)) | color(Color::GrayDark));

    uint8_t *code = reinterpret_cast<uint8_t *>(thread->zero.pc);
    uint16_t s;
    int16_t ss;
    uint8_t j;
    while (code < thread->zero.frame->method->code + thread->zero.frame->method->codeLength)
    {
#define OpArgs16(name)                                                                                                 \
    ss = static_cast<int16_t>(code[1]) << 8 | code[2];                                                                 \
    codelines.push_back(                                                                                               \
        {text(fmt::format("{:02x} {:02x} {:02x}", *code, code[1], code[2])) | color(Color::Green) | flex, separatorEmpty() | flex,             \
         text(fmt::format("{} {}", name, ss)) | color(Color::GrayLight) | flex});                                      \
    code += 3;
#define OpArgu16(name)                                                                                                 \
    s = static_cast<uint16_t>(code[1]) << 8 | code[2];                                                                 \
    codelines.push_back(                                                                                               \
        {text(fmt::format("{:02x} {:02x} {:02x}", *code, code[1], code[2])) | color(Color::Green) | flex, separatorEmpty() | flex,             \
         text(fmt::format("{} {}", name, s)) | color(Color::GrayLight) | flex});                                       \
    code += 3;
#define OpArgu8(name)                                                                                                  \
    j = code[1];                                                                                                       \
    codelines.push_back({text(fmt::format("{:02x} {:02x}", *code, code[1])) | color(Color::Green) | flex, separatorEmpty() | flex,              \
                         text(fmt::format("{} {}", name, (int)j)) | color(Color::GrayLight) | flex});                  \
    code += 2;
#define OpArgu8s8(name)                                                                                                \
    j = code[1];                                                                                                       \
    codelines.push_back(                                                                                               \
        {text(fmt::format("{:02x} {:02x} {:02x}", *code, code[1], code[2])) | color(Color::Green) | flex, separatorEmpty() | flex,             \
         text(fmt::format("{} {} {}", name, (int)j, (int8_t)code[2])) | color(Color::GrayLight) | flex});              \
    code += 3;
#define OpArgv(name)                                                                                                   \
    codelines.push_back({text(fmt::format("{:02x}", *code)) | color(Color::Green) | flex, separatorEmpty() | flex,                             \
                         text(name) | color(Color::GrayLight) | flex});                                                \
    ++code;
#define OpCase(opname, optype)                                                                                         \
    case op_##opname:                                                                                                  \
        optype(#opname);                                                                                               \
        break;
#define OpCaseN(opname1, opname2, opname3, opname4, optype)                                                            \
    OpCase(opname1, optype);                                                                                           \
    OpCase(opname2, optype);                                                                                           \
    OpCase(opname3, optype);                                                                                           \
    OpCase(opname4, optype);

        switch (*code)
        {
            OpCase(nop, OpArgv);
            OpCase(aconst_null, OpArgv);
            OpCase(iconst_i(-1), OpArgv);
            OpCase(iconst_i(0), OpArgv);
            OpCase(iconst_i(1), OpArgv);
            OpCase(iconst_i(2), OpArgv);
            OpCase(iconst_i(3), OpArgv);
            OpCase(iconst_i(4), OpArgv);
            OpCase(iconst_i(5), OpArgv);
            OpCase(lconst_l(0), OpArgv);
            OpCase(lconst_l(1), OpArgv);
            OpCase(fconst_f(0), OpArgv);
            OpCase(fconst_f(1), OpArgv);
            OpCase(fconst_f(2), OpArgv);
            OpCase(dconst_d(0), OpArgv);
            OpCase(dconst_d(1), OpArgv);
            OpCase(bipush, OpArgu8);
            OpCase(sipush, OpArgs16);
            OpCase(ldc, OpArgu8);
            OpCase(ldc_w, OpArgu16);
            OpCase(ldc2_w, OpArgu16);
            OpCaseN(iload, lload, fload, dload, OpArgu8);
            OpCase(aload, OpArgu8);
            OpCaseN(iload_n(0), iload_n(1), iload_n(2), iload_n(3), OpArgv);
            OpCaseN(lload_n(0), lload_n(1), lload_n(2), lload_n(3), OpArgv);
            OpCaseN(fload_n(0), fload_n(1), fload_n(2), fload_n(3), OpArgv);
            OpCaseN(dload_n(0), dload_n(1), dload_n(2), dload_n(3), OpArgv);
            OpCaseN(aload_n(0), aload_n(1), aload_n(2), aload_n(3), OpArgv);
            OpCaseN(iaload, laload, faload, daload, OpArgv);
            OpCaseN(aaload, baload, caload, saload, OpArgv);
            OpCaseN(istore, lstore, fstore, dstore, OpArgu8);
            OpCase(astore, OpArgu8);
            OpCaseN(istore_n(0), istore_n(1), istore_n(2), istore_n(3), OpArgv);
            OpCaseN(lstore_n(0), lstore_n(1), lstore_n(2), lstore_n(3), OpArgv);
            OpCaseN(fstore_n(0), fstore_n(1), fstore_n(2), fstore_n(3), OpArgv);
            OpCaseN(dstore_n(0), dstore_n(1), dstore_n(2), dstore_n(3), OpArgv);
            OpCaseN(astore_n(0), astore_n(1), astore_n(2), astore_n(3), OpArgv);
            OpCaseN(iastore, lastore, fastore, dastore, OpArgv);
            OpCaseN(aastore, bastore, castore, sastore, OpArgv);
            OpCaseN(pop, pop2, dup, dup_x1, OpArgv);
            OpCaseN(dup_x2, dup2, dup2_x1, dup2_x2, OpArgv);
            OpCase(swap, OpArgv);
            OpCaseN(iadd, ladd, fadd, dadd, OpArgv);
            OpCaseN(isub, lsub, fsub, dsub, OpArgv);
            OpCaseN(imul, lmul, fmul, dmul, OpArgv);
            OpCaseN(idiv, ldiv, fdiv, ddiv, OpArgv);
            OpCaseN(irem, lrem, frem, drem, OpArgv);
            OpCaseN(ineg, lneg, fneg, dneg, OpArgv);
            OpCaseN(ishr, lshr, ishl, lshl, OpArgv);
            OpCaseN(iushr, lushr, iand, land, OpArgv);
            OpCaseN(ior, lor, ixor, lxor, OpArgv);
            OpCase(iinc, OpArgu8s8);
            OpCaseN(i2l, i2f, i2d, l2i, OpArgv);
            OpCaseN(l2f, l2d, f2i, f2l, OpArgv);
            OpCaseN(d2i, f2d, d2l, d2f, OpArgv);
            OpCaseN(i2b, i2c, i2s, lcmp, OpArgv);
            OpCaseN(fcmpl, fcmpg, dcmpl, dcmpg, OpArgv);
            OpCaseN(ifeq, ifne, iflt, ifge, OpArgs16);
            OpCase(ifgt, OpArgs16);
            OpCase(ifle, OpArgs16);
            OpCaseN(if_icmpeq, if_icmpne, if_icmplt, if_icmpge, OpArgs16);
            OpCase(if_icmpgt, OpArgs16);
            OpCase(if_icmple, OpArgs16);
            OpCase(if_acmpeq, OpArgs16);
            OpCase(if_acmpne, OpArgs16);
            OpCase(goto, OpArgs16);
            OpCase(jsr, OpArgs16);
            OpCase(ret, OpArgu8);
            // tableswitch
            // lookupswitch
            OpCaseN(ireturn, lreturn, freturn, dreturn, OpArgv);
            OpCase(areturn, OpArgv);
            OpCase(return, OpArgv);
            OpCaseN(getstatic, putstatic, getfield, putfield, OpArgu16);
            OpCaseN(invokespecial, invokevirtual, invokestatic, invokeinterface, OpArgu16);
        case op_invokedynamic:
            OpArgu16("invokedynamic");
            code += 2;
            break;
            OpCase(new, OpArgu16);
            OpCase(newarray, OpArgu8);
            OpCase(anewarray, OpArgu16);
            OpCase(arraylength, OpArgv);
            OpCase(athrow, OpArgv);
            OpCase(checkcast, OpArgu16);
            OpCase(instanceof, OpArgu16);
            OpCase(monitorenter, OpArgv);
            OpCase(monitorexit, OpArgv);
            // wide
            // multianewarray
            OpCase(ifnull, OpArgs16);
            OpCase(ifnonnull, OpArgs16);
            // goto_w
        // jsr_w
        default:
            codelines.push_back({text(fmt::format("{:02x}", *code)) | color(Color::Green) | flex, separatorEmpty() | flex, 
                                 text("<unknown operand>") | color(Color::GrayDark) | flex});
            ++code;
            break;
        }
    }

    return vbox({vbox(codetop) | flex, gridbox(codelines) | flex});
}

Element buildElysiaThreadCode(OMElysiaThread *thread)
{
    if (thread->zero.pc && thread->zero.frame)
    {
        auto c = reinterpret_cast<uint8_t *>(thread->zero.pc);
        auto tt = text(fmt::format("{}", fmt::ptr(thread->zero.pc)));
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
    auto frm = thread->zero.frame;
    while (pp < thread->stackStart)
    {
        auto fptr = reinterpret_cast<uintptr_t>(frm);
        auto cptr = reinterpret_cast<uintptr_t>(pp);

        auto clr = Color::Blue;
        if (fptr > cptr)
        {
            if (fptr - cptr <= frm->method->localLength * sizeof(void *))
            {
                clr = Color::White;
            }
        }
        else
        {
            if (cptr - fptr < sizeof(OMElysiaJavaFrame))
            {
                clr = Color::Green;
            }
            else
            {
                frm = frm->caller;
                continue;
            }
        }

        stk.push_back(
            {separatorEmpty() | flex, text(fmt::format("{}", fmt::ptr(pp))), separatorEmpty() | flex,
             text(fmt::format("{:0" + fmt::format("{}", sizeof(void *) * 2) + "x}", (uintptr_t)*pp)) | color(clr)});
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

    return window(text("Elysia Threads"), hbox(elem));
}

class OMMemoryComponent : public ComponentBase
{
  public:
    OMMemoryComponent()
    {
    }
    Element OnRender()
    {
        return buildMemComp();
    }
};

class OMElysiaThreadDataComponent : public ComponentBase
{
  public:
    OMElysiaThreadDataComponent(OMElysiaThread *thread) : thread(thread)
    {
    }

    Element OnRender()
    {
        return buildElysiaThread(thread);
    }

  private:
    OMElysiaThread *thread;
};

class OMElysiaThreadComponent : public ComponentBase
{
  public:
    OMElysiaThreadComponent()
    {
    }
    Element OnRender()
    {
        DetachAllChildren();
        threadtabs.clear();
        threadContent.clear();
        for (auto &t : threadMap)
        {
            threadtabs.push_back(fmt::format("Thread {}", reinterpret_cast<const void *>(&t.first)));
            threadContent.push_back(std::make_shared<OMElysiaThreadDataComponent>(t.second));
        }
        menuToggle = Menu(&threadtabs, &tabsel);
        menuContainer = Container::Tab(threadContent, &tabsel);
        Add(menuToggle);
        Add(menuContainer);
        return hbox({menuToggle->Render() | flex, separator(), menuContainer->Render()});
    }

  private:
    int tabsel = 0;
    std::vector<std::string> threadtabs;
    std::vector<std::shared_ptr<ComponentBase>> threadContent;

    Component menuToggle;
    Component menuContainer;
};

int main(int argc, const char *argv[])
{
    auto wld = new OMElysiaVirtualWorld;

    std::vector<std::string> tabnames = {"Memory", "ElysiaVM", "Elysia Heap"};

    auto memComp = std::make_shared<OMMemoryComponent>();
    auto elyComp = std::make_shared<OMElysiaThreadComponent>();

    auto cc = Container::Vertical({});
    auto elymemComp = Renderer(cc, [&] {
        return hbox({window(text("Metaspace"), buildElysiaHeapComp(wld->metaspaceHeap)), window(text("main"), buildElysiaHeapComp(wld->mainHeap))});
    });

    int tabsel = 0;
    auto menuToggle = Toggle(&tabnames, &tabsel);
    auto menuContainer = Container::Tab({memComp, elyComp, elymemComp}, &tabsel);

    auto container = Container::Horizontal({menuToggle, menuContainer});
    auto renderer = Renderer(container, [&] {
        animation::RequestAnimationFrame();
        return window(text("Elysia Visualizer"), vbox({menuToggle->Render(), separator(), menuContainer->Render()}));
    });

    auto screen = ScreenInteractive::Fullscreen();
    screen.Loop(renderer);

    delete wld;

    return 0;
}
