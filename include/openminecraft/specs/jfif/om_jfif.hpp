#ifndef OM_JFIF_HPP
#define OM_JFIF_HPP

#include "openminecraft/mem/om_mem_stl_allocator.hpp"
#include <cstdint>
#include <istream>
#include <memory>
#include <vector>
namespace openminecraft::specs::jfif
{
constexpr const char allocatorTag[] = "parser_jfif";
enum OMJfifState
{
    None,
    TagBegin,
    TagContentApp0
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

class OMJfifFile
{
  public:
    OMJfifFile();
    ~OMJfifFile();

    void parse(std::shared_ptr<std::istream> input);

  private:
    OMJfifState state = None;

    OMJfifApp0Header headerApp0;
    std::vector<OMJfifThumbnailPixel, mem::OMStlAllocator<allocatorTag, OMJfifThumbnailPixel>> thumbnail;
};
}; // namespace openminecraft::specs::jfif

#endif
