#ifndef OM_PNG_HPP
#define OM_PNG_HPP

#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include <array>
#include <cstdint>
#include <istream>
#include <memory>
#include <vector>
namespace openminecraft::specs::png
{
constexpr std::array<uint8_t, 8> header = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
constexpr const char allocatorTag[] = "parser_png";

enum OMPngColorType : uint8_t
{
    Grayscale = 0x0,
    RGBTriple = 0x2,
    Palette,
    GrayscaleAlpha,
    RGBA = 0x6
};

struct OMPngChunk
{
    char name[4];
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> data;
    uint32_t crc;
};

struct OMPngHead
{
    uint32_t width;
    uint32_t height;
    uint8_t bitDepth;
    OMPngColorType type;
    uint8_t compressing;
    uint8_t filter;
    uint8_t interlacing;
};

class OMPngFile
{
  public:
    OMPngFile();
    ~OMPngFile();

    void *fetchData();
    int getWidth();
    int getHeight();

    void parse(std::shared_ptr<std::istream> istr);

  private:
    void defilter(int type, std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &, int y);
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> filterCache;

    uint32_t getStride();
    int getBytesPerPixel();

    uint8_t getPaethPred(int a, int b, int c);

    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> dataBuffer;
    OMPngHead head;
    std::vector<int, mem::OMStlAllocator<allocatorTag, int>> palette;

    uint64_t crcTable[256];
    bool crcCalc = false;

    void makeCrcTable();
    uint64_t updateCrc(uint64_t crc, void *, int);
    uint64_t crc(OMPngChunk chunk);
};
} // namespace openminecraft::specs::png

#endif
