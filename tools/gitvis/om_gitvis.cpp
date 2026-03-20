#include <cstdlib>
#include <fstream>
#define TB_IMPL
#include "termbox2/termbox2.h"
#include <iostream>

int main(int argc, char *argv[])
{
    auto exitcode = std::system("git log --reverse --pretty=format:'%H%x1e%an%x1e%ae%x1e%at%x1e%s%x1f' > temp.bin");

    std::ifstream result("temp.bin", std::ios::binary);

    std::string s = "";
    while (result.good())
    {
        char c;
        result.read(&c, 1);

        if (c != 0x1e && c != 0x1f)
        {
            if (c != '\n')
            {
                s += c;
            }
        }
        else
        {
            std::cout << s << std::endl;
            s = "";
        }
    }
    result.close();

    std::remove("temp.bin");

    /*int status = tb_init();
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

    tb_shutdown();*/

    std::cout << "Hello, world!" << std::endl;
    return 0;
}
