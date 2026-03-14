#ifndef OM_JFIF_HPP
#define OM_JFIF_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "openminecraft/specs/blocked/om_blocked_file.hpp"
#include "openminecraft/util/om_util_bitbuffer.hpp"
#include <array>
#include <cstdint>
#include <istream>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

namespace openminecraft::specs::jfif
{

/* zigzag map
 *
 * original
 * 0  1  2  3  4  5  6  7
 * 8  9  10 11 12 13 14 15
 * 16 17 18 19 20 21 22 23
 * 24 25 26 27 28 29 30 31
 * 32 33 34 35 36 37 38 39
 * 40 41 42 43 44 45 46 47
 * 48 49 50 51 52 53 54 55
 * 56 57 58 59 60 61 62 63
 *
 * zigzaged
 * 0  1  5  6  14 15 27 28
 * 2  4  7  13 16 26 29 42
 * 3  8  12 17 25 30 41 43
 * 9  11 18 24 31 40 44 53
 * 10 19 23 32 39 45 52 54
 * 20 22 33 38 46 51 55 60
 * 21 34 37 47 50 56 59 61
 * 35 36 48 49 57 58 62 63
 * */

constexpr std::array<int, 64> unzigzagMap = {0,  1,  5,  6,  14, 15, 27, 28, 2,  4,  7,  13, 16, 26, 29, 42,
                                             3,  8,  12, 17, 25, 30, 41, 43, 9,  11, 18, 24, 31, 40, 44, 53,
                                             10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60,
                                             21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63};

constexpr const char allocatorTag[] = "parser_jfif";
struct OMJfifThumbnailPixel
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct OMJfifApp0Header
{
    uint16_t length;
    char ident[5];
    uint8_t versionMajor;
    uint8_t versionMinor;
    uint8_t unit;

    uint16_t densityX, densityY;
    uint8_t thumbnailX, thumbnailY;
};

struct OMJfifApp2Header
{
    uint16_t length;
};

#pragma pack(1)
struct OMJfifQuantizationTable
{
    uint16_t length;
    uint8_t destination;
    uint8_t table[64];
};
#pragma pack()

struct OMJfifComponentStat
{
    uint8_t id;
    uint8_t factor;
    uint8_t tableId;
};

#pragma pack(1)
struct OMJfifStartOfFrame
{
    uint16_t length;
    uint8_t precision;
    uint16_t height;
    uint16_t width;
    uint8_t components;
};

struct OMJfifHuffmanTable
{
    uint8_t info; // high 4 bit -> DC(0)/AC(1), low 4 bit -> table id
    uint8_t counts[16];
};
#pragma pack()

struct OMJfifHuffmanTableEntry
{
    uint8_t length;
    uint8_t code;
};

#pragma pack(1)
struct OMJfifStartOfScan
{
    uint16_t length;
    uint8_t components;
};
#pragma pack()

struct OMJfifStartOfScanRange
{
    uint8_t spectralBegin, spectralEnd, successive;
};

struct OMJfifStartOfScanSelector
{
    uint8_t selector;
    uint8_t table; // high 4 bits -> dc, low 4 bits -> ac
};

enum OMJfifSectionType : uint8_t
{
    StartOfImage,
    App0Header,
    App2Header,
    Comment,
    QuantizationTable,
    StartOfFrame,
    HuffmanTable,
    ImageData,
    StartOfScan,
    EndOfImage,
    Unknown
};

struct OMJfifBlockStatus
{
    int id;
    uint8_t dcTable;
    uint8_t acTable;
    uint8_t quantizationTable;

    int blockId;
    int scaleX;
    int scaleY;
};

class OMJfifFile : public OMBlockedFile<OMJfifSectionType>
{
  public:
    OMJfifFile();
    ~OMJfifFile();

    static uint16_t readLen(std::shared_ptr<std::istream> input);

    void parseMagic(std::shared_ptr<std::istream>) override;
    bool parseBlockHeader(std::shared_ptr<std::istream>, OMJfifSectionType *) override;

    void parseApp0Header(std::shared_ptr<std::istream>);
    void parseApp2Header(std::shared_ptr<std::istream>);
    void parseComment(std::shared_ptr<std::istream>);
    void parseQuantizationTable(std::shared_ptr<std::istream>);
    void parseStartOfFrame(std::shared_ptr<std::istream>);
    void parseHuffmanTable(std::shared_ptr<std::istream>);
    void parseStartOfScan(std::shared_ptr<std::istream>);
    void parseImageData(std::shared_ptr<std::istream>);

    uint8_t *getData()
    {
        return data.data();
    }
    int getWidth()
    {
        return headerStartOfFrame.width;
    }
    int getHeight()
    {
        return headerStartOfFrame.height;
    }

  private:
    void parseBlock();

    OMJfifApp0Header headerApp0;
    OMJfifApp2Header headerApp2;
    OMJfifStartOfFrame headerStartOfFrame;

    std::unordered_map<int, int> dcTemp;
    std::vector<OMJfifBlockStatus, mem::OMStlAllocator<allocatorTag, OMJfifBlockStatus>> blockids;
    std::vector<OMJfifBlockStatus, mem::OMStlAllocator<allocatorTag, OMJfifBlockStatus>>::iterator currentBlock;

    std::vector<int, mem::OMStlAllocator<allocatorTag, int>> blockData;
    int blockDataIndex = 0;

    OMJfifStartOfScanRange range;
    std::unordered_map<uint8_t, std::vector<std::variant<bool, uint8_t>>> huffmanTable;
    std::unordered_map<uint8_t, OMJfifComponentStat> components;

    std::vector<OMJfifThumbnailPixel, mem::OMStlAllocator<allocatorTag, OMJfifThumbnailPixel>> thumbnail;
    std::vector<OMJfifQuantizationTable, mem::OMStlAllocator<allocatorTag, OMJfifQuantizationTable>> quantizationTable;

    util::OMBitBuffer bitBuffer;

    log::OMLogger logger;

    bool insideImg = false;

    struct
    {
        int mcuid = 0;
        int mcucounts = 0;
        int mcuwidth, mcuheight;
        int mcuxcount, mcuycount;
    } mcuStatus;

    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> data;
};
}; // namespace openminecraft::specs::jfif

#endif
