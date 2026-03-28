#include "openminecraft/specs/jfif/om_jfif.hpp"
#include "openminecraft/binary/om_bin_endians.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
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
    processorMap[App1Header] = [&](std::shared_ptr<std::istream> istr) { parseApp1Header(istr); };
    processorMap[App2Header] = [&](std::shared_ptr<std::istream> istr) { parseApp2Header(istr); };
    processorMap[App13Header] = [&](std::shared_ptr<std::istream> istr) { parseApp13Header(istr); };
    processorMap[App14Header] = [&](std::shared_ptr<std::istream> istr) { parseApp14Header(istr); };
    processorMap[Comment] = [&](std::shared_ptr<std::istream> istr) { parseComment(istr); };
    processorMap[QuantizationTable] = [&](std::shared_ptr<std::istream> istr) { parseQuantizationTable(istr); };
    processorMap[StartOfFrame] = [&](std::shared_ptr<std::istream> istr) { parseStartOfFrame(istr); };
    processorMap[HuffmanTable] = [&](std::shared_ptr<std::istream> istr) { parseHuffmanTable(istr); };
    processorMap[StartOfScan] = [&](std::shared_ptr<std::istream> istr) { parseStartOfScan(istr); };
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

void OMJfifFile::parseApp1Header(std::shared_ptr<std::istream> istr)
{
    istr->read(reinterpret_cast<char *>(&headerApp1), sizeof(OMJfifApp1Header));
    headerApp1.length = binary::be16ToNative(headerApp1.length);
    istr->ignore(headerApp1.length - 2);
}

void OMJfifFile::parseApp2Header(std::shared_ptr<std::istream> istr)
{
    istr->read(reinterpret_cast<char *>(&headerApp2), sizeof(OMJfifApp2Header));
    headerApp2.length = binary::be16ToNative(headerApp2.length);
    istr->ignore(headerApp2.length - 2);
}

void OMJfifFile::parseApp13Header(std::shared_ptr<std::istream> istr)
{
    istr->read(reinterpret_cast<char *>(&headerApp13), sizeof(OMJfifApp13Header));
    headerApp13.length = binary::be16ToNative(headerApp13.length);
    istr->ignore(headerApp13.length - 2);
}

void OMJfifFile::parseApp14Header(std::shared_ptr<std::istream> istr)
{
    istr->read(reinterpret_cast<char *>(&headerApp14), sizeof(OMJfifApp14Header));
    headerApp14.length = binary::be16ToNative(headerApp14.length);
    istr->ignore(headerApp14.length - 2);
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
    uint16_t tbl;
    istr->read(reinterpret_cast<char *>(&tbl), sizeof(uint16_t));
    tbl = binary::be16ToNative(tbl);

    for (int i = 0; i < (tbl - 2) / sizeof(OMJfifQuantizationTable); i++)
    {
        OMJfifQuantizationTable tb;
        istr->read(reinterpret_cast<char *>(&tb), sizeof(OMJfifQuantizationTable));
        quantizationTable.push_back(tb);
    }
}

void OMJfifFile::parseStartOfFrame(std::shared_ptr<std::istream> istr)
{
    istr->read(reinterpret_cast<char *>(&headerStartOfFrame), sizeof(OMJfifStartOfFrame));
    headerStartOfFrame.length = binary::be16ToNative(headerStartOfFrame.length);
    headerStartOfFrame.width = binary::be16ToNative(headerStartOfFrame.width);
    headerStartOfFrame.height = binary::be16ToNative(headerStartOfFrame.height);
    int mcuw = 0;
    int mcuh = 0;
    for (int i = 0; i < headerStartOfFrame.components; i++)
    {
        OMJfifComponentStat st;
        istr->read(reinterpret_cast<char *>(&st), sizeof(OMJfifComponentStat));
        components[st.id] = st;

        auto factor = st.factor;

        mcuw = std::max(mcuw, factor >> 4 & 0xf);
        mcuh = std::max(mcuh, factor & 0xf);
    }
    mcuw *= 8;
    mcuh *= 8;
    mcuStatus.mcuxcount = std::ceil(static_cast<float>(headerStartOfFrame.width) / mcuw);
    mcuStatus.mcuycount = std::ceil(static_cast<float>(headerStartOfFrame.height) / mcuh);
    mcuStatus.mcuwidth = mcuw;
    mcuStatus.mcuheight = mcuh;

    mcuStatus.mcucounts = mcuStatus.mcuxcount * mcuStatus.mcuycount;

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

        int totalcount = 0;
        for (int a = 0; a < 16; a++)
        {
            totalcount += tb.counts[a];
        }

        std::vector<uint8_t> codes;
        codes.resize(totalcount);
        istr->read(reinterpret_cast<char *>(codes.data()), totalcount);

        huffmanTable[tb.info] = std::make_pair(tb, codes);
    }
}

void OMJfifFile::parseStartOfScan(std::shared_ptr<std::istream> istr)
{
    OMJfifStartOfScan sc;
    istr->read(reinterpret_cast<char *>(&sc), sizeof(OMJfifStartOfScan));
    sc.length = binary::be16ToNative(sc.length);

    dcTemp.clear();
    blockids.clear();
    for (int i = 0; i < sc.components; i++)
    {
        OMJfifStartOfScanSelector sel;
        istr->read(reinterpret_cast<char *>(&sel), sizeof(OMJfifStartOfScanSelector));

        auto factor = components[sel.selector].factor;
        for (int by = 0; by < (factor & 0xf); by++)
        {
            for (int bx = 0; bx < (factor >> 4 & 0xf); bx++)
            {
                blockids.push_back({sel.selector, static_cast<uint8_t>(sel.table >> 4),
                                    static_cast<uint8_t>(sel.table & 0xf | 0x10), components[sel.selector].tableId, bx,
                                    by, (factor >> 4 & 0xf), (factor & 0xf)});
            }
        }

        logger.info("0x{:02x} {} {} dc{} ac{}", sel.selector, factor >> 4 & 0xf, factor & 0xf, sel.table >> 4,
                    sel.table & 0xf);

        dcTemp[sel.selector] = 0;
    }

    for (auto &bid : blockids)
    {
        bid.scaleX = mcuStatus.mcuwidth / 8 / bid.scaleX;
        bid.scaleY = mcuStatus.mcuheight / 8 / bid.scaleY;
    }

    istr->read(reinterpret_cast<char *>(&range), sizeof(OMJfifStartOfScanRange));

    logger.info("{} ~ {} freq", range.spectralBegin, range.spectralEnd);

    mcuStatus.mcuid = 0;

    logger.info("{} mcus, {}x{} {}", mcuStatus.mcucounts, mcuStatus.mcuwidth, mcuStatus.mcuheight, blockids.size());

    currentBlock = blockids.begin();

    if (currentBlock->id == 0x01 && range.successive == 0x10)
    {
        istr->ignore(1000000000);
        return;
    }

    switch (imageType)
    {
    case Baseline:
        parseRawBlocksBaseline(istr);
        break;
    case Progressive:
        parseRawBlocksProgressive(istr);
        break;
    default:
        throw std::logic_error("not supported");
    }
}

void OMJfifFile::bumpBlock()
{
    blockDataIndex = range.spectralBegin;
    ++currentBlock;
    if (currentBlock == blockids.end())
    {
        currentBlock = blockids.begin();
        ++mcuStatus.mcuid;
    }
}

glm::mat3 yccToRgb = glm::mat3(1.0f, 1.0f, 1.0f, 0.0f, -0.344136f, 1.772f, 1.402f, -0.714136f, 0.0f);

static void modPixel(uint8_t *data, uint8_t mod, uint8_t channelid)
{
    if ((data[3] & 0b111) == 0b111)
    {
        auto raw = (glm::inverse(yccToRgb) * glm::vec3{data[0], data[1], data[2]}) + glm::vec3{0, 128, 128};

        data[0] = std::clamp(raw.r, 0.0f, 255.0f);
        data[1] = std::clamp(raw.g, 0.0f, 255.0f);
        data[2] = std::clamp(raw.b, 0.0f, 255.0f);
        data[3] = 0b111;
    }

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

    if ((data[3] & 0b111) == 0b111)
    {
        auto raw = yccToRgb * glm::vec3{data[0], data[1] - 128, data[2] - 128};

        data[0] = std::clamp(raw.r, 0.0f, 255.0f);
        data[1] = std::clamp(raw.g, 0.0f, 255.0f);
        data[2] = std::clamp(raw.b, 0.0f, 255.0f);
        data[3] = 0xff;
    }
}

void OMJfifFile::loadBlockCache()
{
    auto hashstr = fmt::format("MCU#{}Cnl#{}Bx{}By{}", mcuStatus.mcuid, currentBlock->id, currentBlock->blockX,
                               currentBlock->blockY);
    auto blkhash = binary::hash::hash_compile_time(hashstr.c_str());

    if (blockDataCache.count(blkhash))
    {
        std::memcpy(blockData.data(), blockDataCache[blkhash].data(), sizeof(int) * range.spectralBegin);
    }
    else
    {
        std::memset(blockData.data(), 0x00, sizeof(int) * 64);
    }
}

void OMJfifFile::saveBlockCache()
{
    auto hashstr = fmt::format("MCU#{}Cnl#{}Bx{}By{}", mcuStatus.mcuid, currentBlock->id, currentBlock->blockX,
                               currentBlock->blockY);
    auto blkhash = binary::hash::hash_compile_time(hashstr.c_str());
    blockDataCache[blkhash] = blockData;
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

    for (int y = 0; y < 8; y++)
    {
        int actualY = mcuy * mcuStatus.mcuheight + (currentBlock->blockY * 8 + y) * currentBlock->scaleY;

        if (actualY >= getHeight())
            break;

        for (int x = 0; x < 8; x++)
        {
            int actualX = mcux * mcuStatus.mcuwidth + (currentBlock->blockX * 8 + x) * currentBlock->scaleX;

            if (actualX >= getWidth())
                break;

            for (int dy = 0; dy < currentBlock->scaleY; dy++)
            {
                for (int dx = 0; dx < currentBlock->scaleX; dx++)
                {
                    int pixid = (actualY + dy) * getWidth() + actualX + dx;
                    if (pixid * 4 >= data.size())
                    {
                        break;
                    }
                    modPixel(data.data() + pixid * 4, target[y * 8 + x], currentBlock->id);
                }
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

    istr->read(reinterpret_cast<char *>(&flag), 1);
    if (flag != 0xff)
    {
        // throw std::logic_error("not 0xff!");
        goto base;
    }

    istr->read(reinterpret_cast<char *>(&flag), 1);
    switch (flag)
    {
    case 0x00:
        *t = Unknown;
        break;
    case 0xd8:
        *t = StartOfImage;
        break;
    case 0xe0:
        *t = App0Header;
        break;
    case 0xe1:
        *t = App1Header;
        break;
    case 0xe2:
        *t = App2Header;
        break;
    case 0xed:
        *t = App13Header;
        break;
    case 0xee:
        *t = App14Header;
        break;
    case 0xfe:
        *t = Comment;
        break;
    case 0xdb:
        *t = QuantizationTable;
        break;
    case 0xc0:
        imageType = Baseline;
        *t = StartOfFrame;
        break;
    case 0xc2:
        imageType = Progressive;
        *t = StartOfFrame;
        break;
    case 0xc3:
        imageType = Lostless;
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
        logger.warn("{:02x} unknown!", flag);
        *t = Unknown;
        throw 0;
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
