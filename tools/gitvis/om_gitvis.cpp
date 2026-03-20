#define TB_IMPL
#include "termbox2/termbox2.h"
#include <cstring>
#include <iostream>

int main(int argc, char *argv[])
{
    int status = tb_init();
    if (status)
    {
        std::cerr << "tb_init() failed " << status << std::endl;
        return 1;
    }

    tb_clear();

    auto width = tb_width();
    auto height = tb_height();

    auto text = "Hello, termbox2!";
    auto textx = (width - std::strlen(text)) / 2;
    auto texty = height / 2;

    tb_printf(textx, texty, TB_CYAN, TB_DEFAULT, text);
    tb_present();

    tb_event e;
    tb_poll_event(&e);

    tb_shutdown();

    std::cout << "Hello, world!" << std::endl;
    return 0;
}
