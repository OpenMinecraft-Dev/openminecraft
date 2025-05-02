#include <cstring>
#include <filesystem>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <ostream>
#include <vector>

std::vector<std::string> fetchFiles(std::string p)
{
    std::vector<std::string> d;
    for (auto e : std::filesystem::directory_iterator(p))
    {
        if (e.is_directory())
        {
            for (auto m : fetchFiles(e.path().string()))
            {
                d.push_back(m);
            }
        }
        else
        {
            d.push_back(e.path().string());
        }
    }
    return d;
}

void writeNumLe(std::ostream &o, uint64_t n)
{
    uint8_t n0 = n >> 0;
    uint8_t n1 = n >> 8;
    uint8_t n2 = n >> 16;
    uint8_t n3 = n >> 24;
    uint8_t n4 = n >> 32;
    uint8_t n5 = n >> 40;
    uint8_t n6 = n >> 48;
    uint8_t n7 = n >> 56;
    o.write((char *)&n0, 1);
    o.write((char *)&n1, 1);
    o.write((char *)&n2, 1);
    o.write((char *)&n3, 1);
    o.write((char *)&n4, 1);
    o.write((char *)&n5, 1);
    o.write((char *)&n6, 1);
    o.write((char *)&n7, 1);
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        return 1;
    }
    std::ofstream out("res.bundle");
    for (auto file : fetchFiles(argv[1]))
    {
        std::ifstream f(file);
        f.seekg(0, std::ios::end);
        uint64_t d = f.tellg();
        auto databuf = new char[d];
        f.seekg(0, std::ios::beg);
        f.read(databuf, d);
        file.replace(0, strlen(argv[1]) + 1, "");
        writeNumLe(out, file.length());
        out.write(file.c_str(), file.length());
        writeNumLe(out, d);
        out.write(databuf, d);
        delete[] databuf;
        f.close();
        std::cout << file << " length: " << d << std::endl;
    }
    out.close();
    return 0;
}