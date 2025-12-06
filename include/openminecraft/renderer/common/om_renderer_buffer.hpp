#ifndef OM_RENDERER_BUFFER_HPP
#define OM_RENDERER_BUFFER_HPP
#include <fmt/format.h>
#include <functional>
#include <openminecraft/log/om_log_common.hpp>

namespace openminecraft::renderer
{
class OMRenderer;
}

namespace openminecraft::renderer::common
{
enum OMBufferUsage
{
    VertexIndex,
    VertexData,
    InstanceData,
    Uniform,
    Misc
};

class OMRendererBuffer
{
  public:
    OMRendererBuffer(OMBufferUsage usage, uint64_t length, OMRenderer *renderer);
    virtual ~OMRendererBuffer();
    const OMBufferUsage usage;
    const uint64_t length;
    bool alwaysMapped = false;

    virtual void updateData(void *src) = 0;

  protected:
    OMRenderer *renderer;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

template <> struct fmt::formatter<openminecraft::renderer::common::OMBufferUsage> : formatter<string_view>
{
    auto format(openminecraft::renderer::common::OMBufferUsage c, format_context &ctx) const -> format_context::iterator
    {
        std::string s;
        switch (c)
        {
        case openminecraft::renderer::common::OMBufferUsage::VertexIndex:
            s = "VertexIndex";
            break;
        case openminecraft::renderer::common::OMBufferUsage::VertexData:
            s = "VertexData";
            break;
        case openminecraft::renderer::common::OMBufferUsage::InstanceData:
            s = "InstanceData";
            break;
        case openminecraft::renderer::common::OMBufferUsage::Misc:
            s = "Misc";
            break;
        case openminecraft::renderer::common::OMBufferUsage::Uniform:
            s = "Un";
            break;
        default:
            s = "<Invalid>";
            break;
        }
        return formatter<string_view>::format(s, ctx);
    }
};

#endif