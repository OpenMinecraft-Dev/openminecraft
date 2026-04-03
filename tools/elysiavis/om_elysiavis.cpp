#include <cstdlib>
#include <iostream>
#include <vector>

#include "fmt/format.h"
#include "ftxui/component/animation.hpp"
#include "ftxui/component/captured_mouse.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/component_options.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/vm/elysia/om_elysia_heap.hpp"
#include "openminecraft/vm/elysia/om_elysia_virtualworld.hpp"

using namespace ftxui;
using namespace openminecraft::vm::elysia;
using openminecraft::mem::castorice::printres;

Element buildMemComp()
{
    std::vector<Element> memcomps;
    printres([&](std::string a, std::string b) { memcomps.push_back(text(fmt::format("{} => {}", a, b))); });
    return vbox({text("Memory"), separatorDashed(), vbox(memcomps)}) | border;
}

int main(int argc, const char *argv[])
{
    auto wld = new OMElysiaVirtualWorld;
    delete wld;

    auto container = Container::Horizontal({});
    auto renderer = Renderer(container, [&] {
        animation::RequestAnimationFrame();

        return vbox(buildMemComp());
    });

    auto screen = ScreenInteractive::Fullscreen();
    screen.Loop(renderer);

    return 0;
}
