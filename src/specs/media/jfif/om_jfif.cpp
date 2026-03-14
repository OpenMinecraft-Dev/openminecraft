#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/specs/jfif/om_jfif_idct.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <variant>

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
    processorMap[StartOfScan] = [&](std::shared_ptr<std::istream> istr) { parseStartOfScan(istr); };
    processorMap[ImageData] = [&](std::shared_ptr<std::istream> istr) { parseImageData(istr); };
    processorMap[EndOfImage] = [&](std::shared_ptr<std::istream> istr) { logger.info("End Of Image"); };
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
        components[st.id] = st;
    }

    data.resize(getWidth() * getHeight() * 4);
}

void OMJfifFile::parseHuffmanTable(std::shared_ptr<std::istream> istr)
{
    uint16_t length;
    istr->read(reinterpret_cast<char *>(&length), sizeof(uint16_t));
    length = binary::be16ToNative(length);
    while (istr->peek() != 0xff)
    {
        OMJfifHuffmanTable tb;
        istr->read(reinterpret_cast<char *>(&tb), sizeof(OMJfifHuffmanTable));

        std::unordered_map<uint8_t, uint8_t> counts;
        for (int i = 0; i < 16; i++)
        {
            counts[i] = tb.counts[i];
        }

        int maxlength = 0;

        for (int i = 0; i < 16; i++)
        {
            if (counts[i])
            {
                maxlength = i + 1;
            }
        }

        auto &htb = huffmanTable[tb.info];
        htb.resize(std::pow(2, maxlength + 1));
        htb[0] = true;
        logger.info("Table #{:02x}", tb.info);

        int currentIdx = 0;
        auto leftNode = [&]() { currentIdx = currentIdx * 2 + 1; };
        auto rightNode = [&]() { currentIdx = currentIdx * 2 + 2; };

        int bitlength = 1;
        int target = 0;

        for (int i = 0; i < 16;)
        {
            if (counts[i] == 0)
            {
                i++;
                continue;
            }
            uint8_t ss;
            istr->read(reinterpret_cast<char *>(&ss), 1);

            if (bitlength < i + 1)
            {
                target <<= (i + 1 - bitlength);
                bitlength = i + 1;
            }

            for (int bb = bitlength - 1; bb >= 0; bb--)
            {
                if ((target >> bb) & 1)
                {
                    rightNode();
                }
                else
                {
                    leftNode();
                }

                htb[currentIdx] = true;
            }

            logger.info("{:b} {} {:02x}", target, currentIdx, ss);
            htb[currentIdx] = ss;
            currentIdx = 0;

            target++;
            counts[i]--;
        }
    }
}

void OMJfifFile::parseStartOfScan(std::shared_ptr<std::istream> istr)
{
    OMJfifStartOfScan sc;
    istr->read(reinterpret_cast<char *>(&sc), sizeof(OMJfifStartOfScan));
    sc.length = binary::be16ToNative(sc.length);

    int mcuw = 0, mcuh = 0;
    dcTemp.clear();
    for (int i = 0; i < sc.components; i++)
    {
        OMJfifStartOfScanSelector sel;
        istr->read(reinterpret_cast<char *>(&sel), sizeof(OMJfifStartOfScanSelector));

        auto factor = components[sel.selector].factor;
        for (int i = 0; i < (factor >> 4 & 0xf) * (factor & 0xf); i++)
        {
            blockids.push_back({sel.selector, static_cast<uint8_t>(sel.table >> 4),
                                static_cast<uint8_t>(sel.table & 0xf | 0x10), components[sel.selector].tableId});
            logger.info("append block : {}", sel.selector);
        }

        mcuw = std::max(mcuw, factor >> 4 & 0xf);
        mcuh = std::max(mcuh, factor & 0xf);

        dcTemp[sel.selector] = 0;
    }

    istr->read(reinterpret_cast<char *>(&range), sizeof(OMJfifStartOfScanRange));

    if (range.spectralEnd != 0x3f)
    {
        throw std::logic_error("not supported");
    }

    mcuw *= 8;
    mcuh *= 8;
    mcuStatus.mcuxcount = std::ceil(static_cast<float>(headerStartOfFrame.width) / mcuw);
    mcuStatus.mcuycount = std::ceil(static_cast<float>(headerStartOfFrame.height) / mcuh);
    mcuStatus.mcuwidth = mcuw;
    mcuStatus.mcuheight = mcuh;

    mcuStatus.mcucounts = mcuStatus.mcuxcount * mcuStatus.mcuycount;

    logger.info("{} mcus", mcuStatus.mcucounts);

    currentBlock = blockids.begin();

    blockData.resize(64);
    blockDataIndex = 0;
    // blockDataPtr = blockData.begin();

    insideImg = true;
    parseImageData(istr);
}

void OMJfifFile::parseImageData(std::shared_ptr<std::istream> istr)
{
    auto pshBit = [&]() -> bool {
        uint8_t c = istr->peek();
        bitBuffer.push(c);
        istr->ignore(1);
        if (c == 0xff)
        {
            if (istr->peek() != 0x00)
            {
                return false;
            }
            else
            {
                istr->ignore(1);
            }
        }
        return true;
    };

    auto requireBits = [&](int b) -> bool {
        while (bitBuffer.bitsAvailable() < b)
        {
            if (!pshBit())
            {
                return false;
            }
        }

        return true;
    };

    auto logpos = [&]() {
        int pp = bitBuffer.bitsAvailable();
        size_t off = istr->tellg();
        while (pp > 0)
        {
            pp -= 8;
            off--;
        }

        logger.info("offset +{:02x}.{}", off, -pp);
    };

nextValue:
    auto huffTable = huffmanTable[blockDataIndex == 0 ? currentBlock->dcTable : currentBlock->acTable];
    uint8_t code;
    int branchidx = 0;
    while (true)
    {
        if (!requireBits(1))
        {
            return;
        }

        if (bitBuffer.popBit())
        {
            branchidx = branchidx * 2 + 2;
        }
        else
        {
            branchidx = branchidx * 2 + 1;
        }

        if (const bool *b = std::get_if<bool>(&huffTable[branchidx]))
        {
            if (!*b)
            {
                logpos();
                throw std::logic_error("bad code!");
            }
        }
        else if (const uint8_t *c = std::get_if<uint8_t>(&huffTable[branchidx]))
        {
            code = *c;
            goto readActual;
        }
    }

readActual:
    uint8_t datalen = blockDataIndex == 0 ? code : (code & 0xf);
    if (!requireBits(datalen))
    {
        return;
    }

    if (blockDataIndex != 0)
    {
        for (int i = 0; i < (code >> 4); i++)
        {
            ++blockDataIndex;
        }
    }

    auto tempval = (int64_t)bitBuffer.popValue(datalen);
    if (datalen)
    {
        if ((tempval >> (datalen - 1)) == 0)
        {
            tempval -= (1 << datalen) - 1;
        }
    }
    if (blockDataIndex == 0)
    {
        tempval += dcTemp[currentBlock->id];
        dcTemp[currentBlock->id] = tempval;
    }
    blockData[blockDataIndex] = tempval;

    if ((code == 0x00 && blockDataIndex != 0) || blockDataIndex >= 63)
    {
        parseBlock();

        blockData.clear();
        blockData.resize(64);
        blockDataIndex = 0;
        ++currentBlock;
        if (currentBlock == blockids.end())
        {
            currentBlock = blockids.begin();
            ++mcuStatus.mcuid;
            blockx = 0;
            blocky = 0;

            if (mcuStatus.mcuid >= mcuStatus.mcucounts)
            {
                insideImg = false;
                bitBuffer.popValue(bitBuffer.bitsAvailable());
                return;
            }
        }
    }
    else
    {
        ++blockDataIndex;
    }
    goto nextValue;
}

void OMJfifFile::parseBlock()
{
    std::array<int, 64> unzig;
    std::array<int, 64> target;
    for (int i = 0; i < 64; i++)
    {
        unzig[i] = blockData[unzigzagMap[i]] * quantizationTable[currentBlock->quantizationTable].table[unzigzagMap[i]];
    }

    jpeg_idct(unzig, target);

    int mcux = mcuStatus.mcuid % mcuStatus.mcuxcount;
    int mcuy = mcuStatus.mcuid / mcuStatus.mcuxcount;

    if (currentBlock->id == 0x01)
    {
        for (int y = 0; y < 8; y++)
        {
            int actualY = mcuy * mcuStatus.mcuheight + blocky * 8 + y;

            if (actualY >= getHeight())
                break;

            for (int x = 0; x < 8; x++)
            {
                int actualX = mcux * mcuStatus.mcuwidth + blockx * 8 + x;

                if (actualX >= getWidth())
                    break;

                int pixid = actualY * getWidth() + actualX;
                data[pixid * 4] = target[y * 8 + x];
                data[pixid * 4 + 1] = data[pixid * 4];
                data[pixid * 4 + 2] = data[pixid * 4];
                data[pixid * 4 + 3] = 0xff;
            }
        }
        blockx++;
        if (blockx >= mcuStatus.mcuwidth / 8)
        {
            blockx = 0;
            blocky++;
        }
    }
}

void OMJfifFile::parseMagic(std::shared_ptr<std::istream> istr)
{
    blockids.clear();
    huffmanTable.clear();
    components.clear();
    thumbnail.clear();
    quantizationTable.clear();
    bitBuffer.popValue(bitBuffer.bitsAvailable());
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

    if (!insideImg)
    {
        istr->read(reinterpret_cast<char *>(&flag), 1);
        if (flag != 0xff)
        {
            goto base;
        }
    }

    istr->read(reinterpret_cast<char *>(&flag), 1);
    switch (flag)
    {
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
