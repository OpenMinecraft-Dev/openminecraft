#include <cstdlib>
#include <iostream>

#include "ftxui/component/animation.hpp"
#include "ftxui/component/captured_mouse.hpp"     // for ftxui
#include "ftxui/component/component.hpp"          // for Button, Renderer, Vertical
#include "ftxui/component/component_base.hpp"     // for ComponentBase
#include "ftxui/component/component_options.hpp"  // for ButtonOption
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/elements.hpp" // for operator|, text, Element, hbox, separator, size, vbox, border, frame, vscroll_indicator, HEIGHT, LESS_THAN
#include "ftxui/screen/color.hpp" // for Color, Color::Default, Color::GrayDark, Color::White

using namespace ftxui;

int main(int argc, const char *argv[])
{
    auto container = Container::Horizontal({});
    auto renderer = Renderer(container, [&] {
        animation::RequestAnimationFrame();
        return vbox({
	           hbox({
                   text("abcd") | flex,
		   separator(),
		   text("abcd") | flex
		   }) | border, 
		   text("ab")
               }) | border;
    });

    auto screen = ScreenInteractive::FitComponent();
    screen.Loop(renderer);

    return 0;
}
