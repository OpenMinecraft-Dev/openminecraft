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
    OMRendererBuffer(OMBufferUsage usage, std::function<void *(OMRendererBuffer *)> alloc, std::function<void(OMRendererBuffer *)> free);
    ~OMRendererBuffer();
    OMBufferUsage const usage;
    void *const actualBuffer;
    void *reserved;

  private:
    std::function<void(OMRendererBuffer *)> free;
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif