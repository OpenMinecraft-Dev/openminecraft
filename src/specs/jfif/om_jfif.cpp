#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include <cstdint>
#include <iostream>

namespace openminecraft::specs::jfif
{
OMJfifFile::OMJfifFile() : logger("OMJfifFile", this)
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
            case 0xe2:
                state = TagContentApp2;
                break;
            case 0xfe:
                state = TagComment;
                break;
            case 0xdb:
                state = TagQuantizationTable;
                break;
            case 0xc0:

                state = TagStartOfFrame;
                break;
            case 0xc4:
                state = TagHuffmanTable;
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

        case TagContentApp2: {
            input->read(reinterpret_cast<char *>(&headerApp2), sizeof(OMJfifApp2Header));
            headerApp2.length = binary::be16ToNative(headerApp2.length);
            input->ignore(headerApp2.length - 2);

            state = None;
            break;
        }

        case TagComment: {
            auto length = readLen(input);
            std::string buf;
            buf.resize(length);
            input->read(buf.data(), length - 2);
            logger.info("JFIF Comment: {}", buf);
            state = None;
            break;
        }

        case TagQuantizationTable: {
            OMJfifQuantizationTable tb;
            input->read(reinterpret_cast<char *>(&tb), sizeof(OMJfifQuantizationTable));
            quantizationTable.push_back(tb);
            state = None;
            break;
        }

        case TagStartOfFrame: {
            input->read(reinterpret_cast<char *>(&headerStartOfFrame), sizeof(OMJfifStartOfFrame));
            headerStartOfFrame.length = binary::be16ToNative(headerStartOfFrame.length);
            headerStartOfFrame.width = binary::be16ToNative(headerStartOfFrame.width);
            headerStartOfFrame.height = binary::be16ToNative(headerStartOfFrame.height);
            for (int i = 0; i < headerStartOfFrame.components; i++)
            {
                OMJfifComponentStat st;
                input->read(reinterpret_cast<char *>(&st), sizeof(OMJfifComponentStat));
                components.push_back(st);
            }

            state = None;
            break;
        }

        case TagHuffmanTable: {
            OMJfifHuffmanTable tb;
            input->read(reinterpret_cast<char *>(&tb), sizeof(OMJfifHuffmanTable));
            tb.length = binary::be16ToNative(tb.length);

            int cnt = 0;
            for (int i = 0; i < 16; i++)
            {
                cnt += tb.counts[i];
            }
            logger.info("{} itts", cnt);
            input->ignore(cnt);
            state = None;
            break;
        }
        }
    }
}

uint16_t OMJfifFile::readLen(std::shared_ptr<std::istream> input)
{
    uint16_t l;
    input->read(reinterpret_cast<char *>(&l), 2);
    return binary::be16ToNative(l);
}
} // namespace openminecraft::specs::jfif
