#ifndef OM_IMAGE_HPP
#define OM_IMAGE_HPP

#include <memory>

namespace openminecraft::specs
{
class OMImage
{
  public:
    virtual ~OMImage()
    {
    }
    virtual int getWidth() = 0;
    virtual int getHeight() = 0;
    virtual void *fetchData() = 0;

    virtual void parseBase(std::shared_ptr<std::istream> input)
    {
    }
};
} // namespace openminecraft::specs

#endif
