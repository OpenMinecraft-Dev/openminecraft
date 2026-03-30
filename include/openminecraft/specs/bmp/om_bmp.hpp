#ifndef OM_BMP_HPP
#define OM_BMP_HPP

#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include <array>
#include <cstdint>
#include <istream>
#include <memory>
#include <vector>
namespace openminecraft::specs::bmp
{
constexpr std::array<uint8_t, 2> header = {'B', 'M'};
constexpr const char allocatorTag[] = "parser_bmp";

struct OMBmpFileHeader
{
    uint32_t size;
    uint16_t reserved1, reserved2;
    uint32_t offset;
};

enum OMBmpFileCompressionType : uint32_t
{
    Rgb,
    Rle4,
    Rle8,
    Bitfields,
    Jpeg,
    Png,
    AlphaBitfields,
    Cmyk,
    CmykRle8,
    CmykRle4
};

struct OMBmpInfoHeader
{
    uint32_t width, height;
    uint16_t planes;
    uint16_t bitCount;
    OMBmpFileCompressionType compression;
    uint32_t sizeImage;
    uint32_t pixelsPerMeterX;
    uint32_t pixelsPerMeterY;
    uint32_t clrUsed;
    uint32_t clrImportant;
};

class OMBmpFile
{
  public:
    OMBmpFile(std::shared_ptr<std::istream> input);
    ~OMBmpFile();

  private:
    OMBmpFileHeader fileHeader;
    OMBmpInfoHeader infoHeader;
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> dataBuffer;
};
} // namespace openminecraft::specs::bmp

#endif
