#include <array>
#include <cstdlib>
#include <fstream>
#define TB_IMPL
#include "fmt/format.h"
#include "termbox2/termbox2.h"
#include <cstring>
#include <iostream>
#include <vector>

int main(int argc, char *argv[])
{
    auto exitcode = std::system("git log --reverse --pretty=format:'%H%x1e%an%x1e%ae%x1e%at%x1e%s%x1f' > temp.bin");

    std::ifstream result("temp.bin", std::ios::binary);

    std::vector<std::array<std::string, 5>> commits = {{}};

    std::string s = "";
    int index = 0;
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
            commits.rbegin()->at(index) = s;
            s = "";
            index++;
        }

        if (c == 0x1f && result.peek() != 0x1f && result.good())
        {
            commits.push_back({});
            index = 0;
        }
    }
    result.close();

    std::remove("temp.bin");

    int status = tb_init();
    if (status)
    {
        std::cerr << "tb_init() failed " << status << std::endl;
        return 1;
    }

    tb_clear();

    auto width = tb_width();
    auto height = tb_height();

    auto text = fmt::format("Hello, termbox2! {} commits", commits.size());
    auto textx = (width - text.size()) / 2;
    auto texty = height / 2;

    tb_printf(textx, texty, TB_CYAN, TB_DEFAULT, text.c_str());
    tb_present();

    tb_event e;
    tb_poll_event(&e);

    tb_shutdown();

    std::cout << "Hello, world!" << std::endl;
    return 0;
}
