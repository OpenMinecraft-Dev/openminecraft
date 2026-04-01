#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <iostream>

int main()
{
    using namespace ftxui;
    auto doc = hbox({
        text("Hello") | border,
        text("FTXUI") | border | flex,
    });

    auto screen = Screen::Create(Dimension::Full(), Dimension::Fit(doc));
    Render(screen, doc);
    screen.Print();
    std::cout << "\n";
}
