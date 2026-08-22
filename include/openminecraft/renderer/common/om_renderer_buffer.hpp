#ifndef OM_RENDERER_BUFFER_HPP
#define OM_RENDERER_BUFFER_HPP
#include "openminecraft/renderer/om_renderer_object.hpp"
#include <fmt/format.h>
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
    Indirect,
    ShaderStorage,
    UniformTexel,
    Misc
};

class OMRendererBuffer : public OMRendererObject
{
  public:
    OMRendererBuffer(OMBufferUsage usage, uint64_t length, OMRenderer *renderer);
    virtual ~OMRendererBuffer();
    const OMBufferUsage usage;
    const uint64_t length;
    void *data = nullptr;
    bool alwaysMapped = false;

    virtual void updateData(void *src) = 0;
    virtual void updateDataPart(void *src, uint64_t offset, uint64_t length) = 0;
    virtual void copyTo(OMRendererBuffer *) = 0;
    auto objType() -> OMRendererObjectType override
    {
        return DataBuffer;
    }

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
            s = "Uniform";
            break;
        case openminecraft::renderer::common::OMBufferUsage::Indirect:
            s = "Indirect";
            break;
        case openminecraft::renderer::common::OMBufferUsage::ShaderStorage:
            s = "ShaderStorage";
            break;
        case openminecraft::renderer::common::OMBufferUsage::UniformTexel:
            s = "UniformTexel";
            break;
        default:
            s = "<Invalid>";
            break;
        }
        return formatter<string_view>::format(s, ctx);
    }
};

#endif
