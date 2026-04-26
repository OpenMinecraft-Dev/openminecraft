#ifndef OM_PNG_HPP
#define OM_PNG_HPP

#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include "openminecraft/specs/abstracts/om_blocked_file.hpp"
#include "openminecraft/specs/abstracts/om_image.hpp"
#include "openminecraft/specs/zlib/om_zlib_inflate.hpp"
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

struct OMPngInterlaceInfo
{
    uint32_t offsetx;
    uint32_t offsety;
    uint32_t stridex;
    uint32_t stridey;

    uint8_t areax;
    uint8_t areay;
};

constexpr std::array<OMPngInterlaceInfo, 7> pngAdam7 = {
    OMPngInterlaceInfo{0, 0, 8, 8, 8, 8}, OMPngInterlaceInfo{4, 0, 8, 8, 4, 8}, OMPngInterlaceInfo{0, 4, 4, 8, 4, 4},
    OMPngInterlaceInfo{2, 0, 4, 4, 2, 4}, OMPngInterlaceInfo{0, 2, 2, 4, 2, 2}, OMPngInterlaceInfo{1, 0, 2, 2, 1, 2},
    OMPngInterlaceInfo{0, 1, 1, 2, 1, 1}};

enum OMPngChunkType
{
    Header,
    PaletteDefine,
    PaletteTransparency,
    ImageData,
    ImageEnd,
    Unknown
};

class OMPngFile : public OMBlockedFile<OMPngChunkType>, public OMImage
{
  public:
    OMPngFile();
    ~OMPngFile();

    void *fetchData() override;
    int getWidth() override;
    int getHeight() override;

    void parseBase(std::shared_ptr<std::istream> in) override
    {
        parse(in);
    }

  private:
    void parseMagic(std::shared_ptr<std::istream>) override;
    bool parseBlockHeader(std::shared_ptr<std::istream>, OMPngChunkType *) override;

    void parseIHDR(std::shared_ptr<std::istream> istr);
    void parsePTLE(std::shared_ptr<std::istream> istr);
    void parseTRNS(std::shared_ptr<std::istream> istr);
    void parseIDAT(std::shared_ptr<std::istream> istr);

    std::pair<uint32_t, uint32_t> getAdamPassSize(int pass);
    void defilterAdam(int type, std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &, int y, int pass);
    void writeIntoBufferAdam(std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &, int pass, int y);
    void defilter(int type, std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &, int y);
    void writeIntoBuffer(std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> &);
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> filterCache;

    uint32_t getStride(int width);
    int getBytesPerPixel();

    OMPngChunk currentChunk;
    int y, pass;
    std::vector<uint8_t, mem::OMStlAllocator<allocatorTag, uint8_t>> dataBuffer, unzippedBuffer;
    std::shared_ptr<zlib::OMZLibInflater> inflater;
    OMPngHead head;
    std::vector<int, mem::OMStlAllocator<allocatorTag, int>> palette;

    uint64_t crc(OMPngChunk chunk);
};
} // namespace openminecraft::specs::png

#endif
