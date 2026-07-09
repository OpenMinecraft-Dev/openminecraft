#ifndef OM_IMAGE_HPP
#define OM_IMAGE_HPP

#include <memory>

namespace openminecraft::specs
{
class OMImage
{
  public:
    virtual ~OMImage() = default;
    virtual auto getWidth() -> int = 0;
    virtual auto getHeight() -> int = 0;
    virtual auto fetchData() -> void * = 0;

    virtual void parseBase(std::shared_ptr<std::istream> input)
    {
    }
};
} // namespace openminecraft::specs

#endif
