#ifndef OM_PNG_HPP
#define OM_PNG_HPP

#include <array>
#include <cstdint>
#include <istream>
#include <memory>
#include <vector>
namespace openminecraft::specs::png
{
constexpr std::array<uint8_t, 8> header = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

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
    uint64_t length;
    char name[4];
    void *data;
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
    OMPngFile(std::shared_ptr<std::istream> istr);
    ~OMPngFile();

    void *fetchData();
    int getWidth();
    int getHeight();

  private:
    uint32_t getStride();
    int getBytesPerPixel();

    uint8_t getBufferA(int y, int x);
    uint8_t getBufferB(int y, int x);
    uint8_t getBufferC(int y, int x);
    uint8_t getPaethPred(int a, int b, int c);

    void *dataBuffer;
    OMPngHead head;
    std::vector<int> palette;

    uint64_t crcTable[256];
    bool crcCalc = false;

    void makeCrcTable();
    uint64_t updateCrc(uint64_t crc, void *, int);
    uint64_t crc(OMPngChunk chunk);
};
} // namespace openminecraft::specs::png

#endif
