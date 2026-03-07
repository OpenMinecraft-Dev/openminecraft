#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include <cstdint>
#include <iostream>
#include <istream>
#include <memory>

namespace openminecraft::specs::jfif
{
OMJfifFile::OMJfifFile() : logger("OMJfifFile", this)
{
    processorMap[StartOfImage] = [](std::shared_ptr<std::istream>) {};
    processorMap[App0Header] = [&](std::shared_ptr<std::istream> istr) { parseApp0Header(istr); };
    processorMap[App2Header] = [&](std::shared_ptr<std::istream> istr) { parseApp2Header(istr); };
    processorMap[Comment] = [&](std::shared_ptr<std::istream> istr) { parseComment(istr); };
    processorMap[QuantizationTable] = [&](std::shared_ptr<std::istream> istr) { parseQuantizationTable(istr); };
    processorMap[StartOfFrame] = [&](std::shared_ptr<std::istream> istr) { parseStartOfFrame(istr); };
    processorMap[HuffmanTable] = [&](std::shared_ptr<std::istream> istr) { parseHuffmanTable(istr); };
}
OMJfifFile::~OMJfifFile()
{
}

void OMJfifFile::parseApp0Header(std::shared_ptr<std::istream> istr)
{
    istr->read(reinterpret_cast<char *>(&headerApp0), sizeof(OMJfifApp0Header));
    headerApp0.length = binary::be16ToNative(headerApp0.length);
    headerApp0.densityX = binary::be16ToNative(headerApp0.densityX);
    headerApp0.densityY = binary::be16ToNative(headerApp0.densityY);

    thumbnail.resize(headerApp0.thumbnailX * headerApp0.thumbnailY);

    istr->read(reinterpret_cast<char *>(thumbnail.data()), thumbnail.size() * sizeof(OMJfifThumbnailPixel));
}

void OMJfifFile::parseApp2Header(std::shared_ptr<std::istream> istr)
{
    istr->read(reinterpret_cast<char *>(&headerApp2), sizeof(OMJfifApp2Header));
    headerApp2.length = binary::be16ToNative(headerApp2.length);
    istr->ignore(headerApp2.length - 2);
}

void OMJfifFile::parseComment(std::shared_ptr<std::istream> istr)
{
    auto length = readLen(istr);
    std::string buf;
    buf.resize(length);
    istr->read(buf.data(), length - 2);
    logger.info("JFIF Comment: {}", buf);
}

void OMJfifFile::parseQuantizationTable(std::shared_ptr<std::istream> istr)
{
    OMJfifQuantizationTable tb;
    istr->read(reinterpret_cast<char *>(&tb), sizeof(OMJfifQuantizationTable));
    quantizationTable.push_back(tb);
}

void OMJfifFile::parseStartOfFrame(std::shared_ptr<std::istream> istr)
{
    istr->read(reinterpret_cast<char *>(&headerStartOfFrame), sizeof(OMJfifStartOfFrame));
    headerStartOfFrame.length = binary::be16ToNative(headerStartOfFrame.length);
    headerStartOfFrame.width = binary::be16ToNative(headerStartOfFrame.width);
    headerStartOfFrame.height = binary::be16ToNative(headerStartOfFrame.height);
    for (int i = 0; i < headerStartOfFrame.components; i++)
    {
        OMJfifComponentStat st;
        istr->read(reinterpret_cast<char *>(&st), sizeof(OMJfifComponentStat));
        components.push_back(st);
    }
}

void OMJfifFile::parseHuffmanTable(std::shared_ptr<std::istream> istr)
{
    OMJfifHuffmanTable tb;
    istr->read(reinterpret_cast<char *>(&tb), sizeof(OMJfifHuffmanTable));
    tb.length = binary::be16ToNative(tb.length);

    std::unordered_map<uint8_t, uint8_t> counts;
    for (int i = 0; i < 16; i++)
    {
        counts[i] = tb.counts[i];
    }

    for (int i = 0; i < 16;)
    {
        if (counts[i] == 0)
        {
            i++;
            continue;
        }
        uint8_t ss;
        istr->read(reinterpret_cast<char *>(&ss), 1);
        logger.info("{} {} l: 0x{:02x}", tb.info, i + 1, ss);
        counts[i]--;
    }
}

void OMJfifFile::parseMagic(std::shared_ptr<std::istream> istr)
{
}
bool OMJfifFile::parseBlockHeader(std::shared_ptr<std::istream> istr, OMJfifSectionType *t)
{
    uint8_t flag;
base:
    if (!istr->good())
    {
        *t = Unknown;
        return false;
    }

    istr->read(reinterpret_cast<char *>(&flag), 1);
    if (flag != 0xff)
    {
        goto base;
    }

    istr->read(reinterpret_cast<char *>(&flag), 1);
    switch (flag)
    {
    case 0x00:
        *t = ImageData;
        break;
    case 0xd8:
        *t = StartOfImage;
        break;
    case 0xe0:
        *t = App0Header;
        break;
    case 0xe2:
        *t = App2Header;
        break;
    case 0xfe:
        *t = Comment;
        break;
    case 0xdb:
        *t = QuantizationTable;
        break;
    case 0xc0:
        *t = StartOfFrame;
        break;
    case 0xc4:
        *t = HuffmanTable;
        break;
    case 0xda:
        *t = StartOfScan;
        break;
    case 0xd9:
        *t = EndOfImage;
        break;
    default:
        *t = Unknown;
        break;
    }

    return true;
}

uint16_t OMJfifFile::readLen(std::shared_ptr<std::istream> input)
{
    uint16_t l;
    input->read(reinterpret_cast<char *>(&l), 2);
    return binary::be16ToNative(l);
}
} // namespace openminecraft::specs::jfif
