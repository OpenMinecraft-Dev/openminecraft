#ifndef OM_JFIF_HPP
#define OM_JFIF_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "openminecraft/specs/blocked/om_blocked_file.hpp"
#include "openminecraft/util/om_util_bitbuffer.hpp"
#include <cstdint>
#include <istream>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>
namespace openminecraft::specs::jfif
{
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

  private:
    OMJfifApp0Header headerApp0;
    OMJfifApp2Header headerApp2;
    OMJfifStartOfFrame headerStartOfFrame;

    std::vector<OMJfifBlockStatus, mem::OMStlAllocator<allocatorTag, OMJfifBlockStatus>> blockids;
    std::vector<OMJfifBlockStatus, mem::OMStlAllocator<allocatorTag, OMJfifBlockStatus>>::iterator currentBlock;

    std::vector<int, mem::OMStlAllocator<allocatorTag, int>> blockData;
    std::vector<int, mem::OMStlAllocator<allocatorTag, int>>::iterator blockDataPtr;

    OMJfifStartOfScanRange range;
    std::unordered_map<uint8_t, std::vector<std::variant<bool, uint8_t>>> huffmanTable;
    std::unordered_map<uint8_t, OMJfifComponentStat> components;

    std::vector<OMJfifThumbnailPixel, mem::OMStlAllocator<allocatorTag, OMJfifThumbnailPixel>> thumbnail;
    std::vector<OMJfifQuantizationTable, mem::OMStlAllocator<allocatorTag, OMJfifQuantizationTable>> quantizationTable;

    util::OMBitBuffer bitBuffer;

    log::OMLogger logger;

    bool insideImg = false;
    int mcuid = 0;
    int mcucounts = 0;
};
}; // namespace openminecraft::specs::jfif

#endif
