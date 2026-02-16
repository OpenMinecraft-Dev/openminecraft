#ifndef OM_PNG_HPP
#define OM_PNG_HPP

#include <array>
#include <cstdint>
#include <istream>
#include <memory>
namespace openminecraft::specs::png
{
constexpr std::array<uint8_t, 8> header = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

struct OMPngChunk
{
    uint64_t length;
    char name[4];
    void *data;
    uint32_t crc;
};

class OMPngFile
{
  public:
    OMPngFile(std::shared_ptr<std::istream> istr);
    ~OMPngFile();
};
} // namespace openminecraft::specs::png

#endif
