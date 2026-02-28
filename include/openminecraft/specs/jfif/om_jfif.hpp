#ifndef OM_JFIF_HPP
#define OM_JFIF_HPP

#include <cstdint>
#include <istream>
#include <memory>
namespace openminecraft::specs::jfif
{
enum OMJfifState
{
    None,
    TagBegin,
    TagContentApp0
};

struct OMJfifApp0Header
{
    uint16_t length;
    char ident[5];
    uint8_t versionMajor;
    uint8_t versionMinor;
    uint8_t unit;

    uint16_t densityX, densityY;
    uint8_t thunbnailX, thunbnailY;

    struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } thunbnail[0];
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
};
}; // namespace openminecraft::specs::jfif

#endif
