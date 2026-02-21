#ifndef OM_RENDERER_EXCEPTION
#define OM_RENDERER_EXCEPTION

#include <stdexcept>
#include <string>

namespace openminecraft::renderer
{
class OMRendererException : public std::exception
{
  public:
    OMRendererException(std::string s) : msg(s)
    {
    }

    ~OMRendererException() override
    {
    }

    const char *what() const noexcept override
    {
        return msg.c_str();
    }

  private:
    std::string msg;
};
} // namespace openminecraft::renderer

#endif
