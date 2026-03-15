#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/specs/jfif/om_jfif_idct.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <istream>
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
        for (int ii = 0; ii < (factor >> 4 & 0xf) * (factor & 0xf); ii++)
        {
            blockids.push_back({sel.selector, static_cast<uint8_t>(sel.table >> 4),
                                static_cast<uint8_t>(sel.table & 0xf | 0x10), components[sel.selector].tableId, ii,
                                (factor >> 4 & 0xf), (factor & 0xf)});
        }

        mcuw = std::max(mcuw, factor >> 4 & 0xf);
        mcuh = std::max(mcuh, factor & 0xf);

        dcTemp[sel.selector] = 0;
    }

    for (auto &bid : blockids)
    {
        bid.scaleX = mcuw / bid.scaleX;
        bid.scaleY = mcuh / bid.scaleY;
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
    mcuStatus.mcuid = 0;

    logger.info("{} mcus", mcuStatus.mcucounts);

    currentBlock = blockids.begin();

    blockData.resize(64);
    blockDataIndex = 0;

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

glm::mat3 yccToRgb = glm::mat3(1.0f, 1.0f, 1.0f, 0.0f, -0.344136f, 1.772f, 1.402f, -0.714136f, 0.0f);

static void modPixel(uint8_t *data, uint8_t mod, uint8_t channelid)
{
    switch (channelid)
    {
    case 1:
        data[0] = mod;
        data[3] |= 0b1;
        break;
    case 2:
        data[1] = mod;
        data[3] |= 0b10;
        break;
    case 3:
        data[2] = mod;
        data[3] |= 0b100;
        break;
    default:
        break;
    }

    if ((data[3] & 0b100) == 0b100)
    {
        /*int r = data[0] + ((1402 * (data[2] - 128)) >> 10);
        int g = data[0] - ((344 * (data[1] - 128) + 714 * (data[2] - 128)) >> 10);
        int b = data[0] + ((1772 * (data[1] - 128)) >> 10);*/

        glm::vec3 raw = {data[0], data[1] - 128, data[2] - 128};
        raw = yccToRgb * raw;

        data[0] = std::clamp(raw.r, 0.0f, 255.0f);
        data[1] = std::clamp(raw.g, 0.0f, 255.0f);
        data[2] = std::clamp(raw.b, 0.0f, 255.0f);
        data[3] = 0xff;
    }
    /*glm::vec3 raw = {data[pid * 4], data[pid * 4 + 1], data[pid * 4 + 2]};
    raw = glm::inverse(yccToRgb) * raw;
    raw += glm::vec3{0, 128, 128};

    switch (channelid)
    {
    case 1:
        raw.x = mod;
        break;
    case 2:
        raw.y = mod;
        break;
    case 3:
        raw.z = mod;
        break;
    default:
        break;
    }

    raw -= glm::vec3{0, 128, 128};
    raw = yccToRgb * raw;

    data[pid * 4] = std::clamp(raw.x, 0.0f, 255.0f);
    data[pid * 4 + 1] = std::clamp(raw.y, 0.0f, 255.0f);
    data[pid * 4 + 2] = std::clamp(raw.z, 0.0f, 255.0f);
    data[pid * 4 + 3] = 0xff;*/
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

    int blockx = currentBlock->blockId % (mcuStatus.mcuwidth / 8);
    int blocky = currentBlock->blockId / (mcuStatus.mcuwidth / 8);

    if (true)
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
                modPixel(data.data() + pixid * 4, target[y * 8 + x], currentBlock->id);
            }
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
