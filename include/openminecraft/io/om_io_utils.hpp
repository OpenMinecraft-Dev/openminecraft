#ifndef OM_IO_UTILS_HPP
#define OM_IO_UTILS_HPP
#include <cstdint>
#include <iostream>
#include <vector>

namespace openminecraft::io
{
static auto readOnce(std::istream *f) -> std::vector<uint8_t>
{
    f->seekg(0, std::ios::end);
    auto length = f->tellg();
    f->seekg(0, std::ios::beg);
    std::vector<uint8_t> data(length);
    f->read(reinterpret_cast<char *>(data.data()), length);
    return data;
}
} // namespace openminecraft::io

#endif
