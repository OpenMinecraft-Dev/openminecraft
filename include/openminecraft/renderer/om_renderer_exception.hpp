#ifndef OM_RENDERER_EXCEPTION
#define OM_RENDERER_EXCEPTION

#include <string>
#include <utility>

namespace openminecraft::renderer
{
class OMRendererException : public std::exception
{
  public:
    OMRendererException(std::string s) : msg(std::move(s))
    {
    }

    ~OMRendererException() override = default;

    [[nodiscard]] auto what() const noexcept -> const char * override
    {
        return msg.c_str();
    }

  private:
    std::string msg;
};
} // namespace openminecraft::renderer

#endif
