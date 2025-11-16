#ifndef OM_RENDERER_BUFFER_HPP
#define OM_RENDERER_BUFFER_HPP
#include <functional>
#include <openminecraft/log/om_log_common.hpp>

namespace openminecraft::renderer::common
{
enum OMBufferUsage
{
    VertexIndex,
    VertexData,
    InstanceData,
    Texture,
    Misc
};

class OMRendererBuffer
{
  public:
    OMRendererBuffer(OMBufferUsage usage, uint64_t length);
    virtual ~OMRendererBuffer();
    const OMBufferUsage usage;
    const uint64_t length;

    virtual void initialize() = 0;
    virtual void release() = 0;

    virtual void updateData(void *src) = 0;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif