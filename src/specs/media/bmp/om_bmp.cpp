#include "openminecraft/specs/bmp/om_bmp.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include <array>
#include <iostream>
#include <istream>
#include <memory>
#include <stdexcept>

namespace openminecraft::specs::bmp
{
OMBmpFile::OMBmpFile(std::shared_ptr<std::istream> input)
{
    std::array<uint8_t, 2> hd;
    input->read(reinterpret_cast<char *>(hd.data()), 2);
    if (hd != header)
    {
        throw std::runtime_error("unknown file header!");
    }

    input->read(reinterpret_cast<char *>(&fileHeader), sizeof(OMBmpFileHeader));
    fileHeader.offset = binary::le32ToNative(fileHeader.offset);
    fileHeader.size = binary::le32ToNative(fileHeader.size);

    uint32_t hdlen;
    input->read(reinterpret_cast<char *>(&hdlen), sizeof(uint32_t));
    input->read(reinterpret_cast<char *>(&infoHeader), sizeof(OMBmpInfoHeader));

    if (infoHeader.compression != Rgb)
    {
        throw std::logic_error("not supported");
    }

    std::cout << fileHeader.offset << std::endl;
    std::cout << infoHeader.width << " " << infoHeader.height << std::endl;
}
OMBmpFile::~OMBmpFile()
{
}
} // namespace openminecraft::specs::bmp
