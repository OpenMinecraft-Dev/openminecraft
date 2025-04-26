#include "openminecraft/boot/om_boot.hpp"

int main(int argc, char **argv)
{
    std::vector<std::string> a(argc);
    for (int i = 0; i < argc; i++)
    {
        a.push_back(argv[i]);
    }
    return openminecraft::boot::boot(a);
}