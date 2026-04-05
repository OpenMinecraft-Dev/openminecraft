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
#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
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

int main(int argc, const char *argv[])
{
    auto wld = new OMElysiaVirtualWorld;

    auto container = Container::Horizontal({});
    auto renderer = Renderer(container, [&] {
        animation::RequestAnimationFrame();

        return vbox({buildMemComp(), window(text("ElysiaVM"), buildElysiaHeapComp2(wld))});
    });

    auto screen = ScreenInteractive::FitComponent();
    screen.Loop(renderer);

    delete wld;

    return 0;
}
