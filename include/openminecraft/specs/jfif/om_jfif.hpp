#ifndef OM_JFIF_HPP
#define OM_JFIF_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include <cstdint>
#include <istream>
#include <memory>
#include <unordered_map>
#include <vector>
namespace openminecraft::specs::jfif
{
constexpr const char allocatorTag[] = "parser_jfif";
enum OMJfifState
{
    None,
    TagBegin,
    TagContentApp0,
    TagContentApp2,
    TagComment,
    TagQuantizationTable,
    TagStartOfFrame,
    TagHuffmanTable
};

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

struct OMJfifQuantizationTable
{
    uint16_t length;
    uint8_t destination;
    uint8_t table[64];
};

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
    uint16_t length;
    uint8_t info; // high 4 bit -> DC/AC, low 4 bit -> table id
    uint8_t counts[16];
};
#pragma pack()

struct OMJfifHuffmanTableEntry
{
    uint8_t length;
    uint8_t code;
};

class OMJfifFile
{
  public:
    OMJfifFile();
    ~OMJfifFile();

    void parse(std::shared_ptr<std::istream> input);
    static uint16_t readLen(std::shared_ptr<std::istream> input);

  private:
    OMJfifState state = None;

    OMJfifApp0Header headerApp0;
    OMJfifApp2Header headerApp2;
    OMJfifStartOfFrame headerStartOfFrame;

    std::unordered_map<uint8_t, OMJfifHuffmanTableEntry> huffmanTable;
    std::vector<OMJfifComponentStat, mem::OMStlAllocator<allocatorTag, OMJfifComponentStat>> components;
    std::vector<OMJfifThumbnailPixel, mem::OMStlAllocator<allocatorTag, OMJfifThumbnailPixel>> thumbnail;
    std::vector<OMJfifQuantizationTable, mem::OMStlAllocator<allocatorTag, OMJfifQuantizationTable>> quantizationTable;

    log::OMLogger logger;
};
}; // namespace openminecraft::specs::jfif

#endif
