#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include <cstdint>
#include <iostream>

namespace openminecraft::specs::jfif
{
OMJfifFile::OMJfifFile()
{
}
OMJfifFile::~OMJfifFile()
{
}

void OMJfifFile::parse(std::shared_ptr<std::istream> input)
{
    while (input->good())
    {
        uint8_t b;

        switch (state)
        {
        case None: {
            input->read(reinterpret_cast<char *>(&b), 1);
            if (b == 0xff)
            {
                state = TagBegin;
            }
            break;
        }

        case TagBegin: {
            input->read(reinterpret_cast<char *>(&b), 1);
            switch (b)
            {
            case 0x00:
                state = None;
                break;
            case 0xd8:
                state = None;
                break;
            case 0xe0:
                state = TagContentApp0;
                break;
            default:
                std::cout << "0x" << std::hex << (int)b << std::dec << std::endl;
                state = None;
                break;
            }
            break;
        }

        case TagContentApp0: {
            input->read(reinterpret_cast<char *>(&headerApp0), sizeof(OMJfifApp0Header));
            headerApp0.length = binary::be16ToNative(headerApp0.length);
            headerApp0.densityX = binary::be16ToNative(headerApp0.densityX);
            headerApp0.densityY = binary::be16ToNative(headerApp0.densityY);

            thumbnail.resize(headerApp0.thumbnailX * headerApp0.thumbnailY);

            input->read(reinterpret_cast<char *>(thumbnail.data()), thumbnail.size() * sizeof(OMJfifThumbnailPixel));

            state = None;
            break;
        }
        }
    }
}
} // namespace openminecraft::specs::jfif
