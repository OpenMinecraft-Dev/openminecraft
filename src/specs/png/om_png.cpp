#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "zlib.h"
#include <cstdint>
#include <iostream>
#include <istream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace openminecraft::specs::png
{
OMPngFile::OMPngFile(std::shared_ptr<std::istream> istr)
{
    std::array<uint8_t, 8> hd = {};
    istr->read(reinterpret_cast<char *>(hd.data()), 8);

    if (hd != header)
    {
        throw std::logic_error("Bad png header!");
    }

    std::vector<OMPngChunk> chunks;
    uint32_t actualLength = 0;
    while (istr->good())
    {
        OMPngChunk cnk;

        uint32_t length;
        istr->read(reinterpret_cast<char *>(&length), 4);
        length = binary::be32ToNative(length);

        cnk.length = length;
        istr->read(cnk.name, 4);

        auto datac = malloc(length);
        istr->read(reinterpret_cast<char *>(datac), length);
        cnk.data = datac;

        istr->read(reinterpret_cast<char *>(&cnk.crc), 4);

        chunks.push_back(cnk);

        if (cnk.name[0] == 'I' && cnk.name[1] == 'E' && cnk.name[2] == 'N' && cnk.name[3] == 'D')
        {
            break;
        }

        if (cnk.name[0] == 'I' && cnk.name[1] == 'D' && cnk.name[2] == 'A' && cnk.name[3] == 'T')
        {
            actualLength += length;
        }
    }

    std::cout << actualLength << "bytes";
}

OMPngFile::~OMPngFile()
{
}
} // namespace openminecraft::specs::png
