#include "openminecraft/specs/jfif/om_jfif.hpp"
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace openminecraft::specs::jfif
{
uint8_t OMJfifFile::fetchCode(uint8_t tableid, std::function<bool()> f)
{
    auto const &table = huffmanTable[tableid].first;
    auto const &symbols = huffmanTable[tableid].second;

    uint32_t first_code[17] = {0};
    uint32_t start_index[17] = {0};
    int total_symbols = 0;
    for (int len = 1; len <= 16; ++len)
    {
        start_index[len] = total_symbols;
        total_symbols += table.counts[len - 1];
        if (len == 1)
        {
            first_code[len] = 0;
        }
        else
        {
            first_code[len] = (first_code[len - 1] + table.counts[len - 2]) << 1;
        }
    }

    uint32_t code = 0;
    int bits_read = 0;

    while (bits_read < 16)
    {
        if (!f())
        {
            throw std::logic_error("no more bits!");
        }
        int bit = bitBuffer.popBit();
        code = (code << 1) | bit;
        bits_read++;

        if (bits_read <= 16 && table.counts[bits_read - 1] > 0)
        {
            uint32_t first = first_code[bits_read];
            uint32_t last = first + table.counts[bits_read - 1] - 1;
            if (code >= first && code <= last)
            {
                uint32_t offset = code - first;
                uint32_t symbol_index = start_index[bits_read] + offset;
                if (symbol_index < symbols.size())
                {
                    return symbols[symbol_index];
                }
                else
                {
                    throw std::logic_error("index out of bound!");
                }
            }
        }
    }

    throw std::logic_error("more than 16 bits!");
}
} // namespace openminecraft::specs::jfif
