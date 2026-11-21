#ifndef OM_RENDERER_LAYER_OPENGL_FRAME_CONTEXT_HPP
#define OM_RENDERER_LAYER_OPENGL_FRAME_CONTEXT_HPP

#include "openminecraft/renderer/common/om_renderer_frame_context.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include <vector>

namespace openminecraft::renderer::opengl
{
class OMRendererOpenGL;

// OpenGL has no explicit frame fences; the frame context is only a task
// execution boundary. begin()/end() are intentionally empty. beforeFrame and
// afterFrame remain on OMRendererOpenGL.
class OMRendererFrameContextOpenGL : public openminecraft::renderer::common::OMRendererFrameContext
{
  public:
    OMRendererFrameContextOpenGL(OMRendererOpenGL *renderer, uint32_t index) : renderer(renderer), frameIndex(index)
    {
    }

    auto index() const -> uint32_t override
    {
        return frameIndex;
    }

    void begin() override
    {
    }

    void end() override
    {
    }

    void executeTasks(const std::vector<std::vector<openminecraft::renderer::common::OMRendererTask *>> &tasks);

  private:
    OMRendererOpenGL *renderer;
    uint32_t frameIndex;
};
} // namespace openminecraft::renderer::opengl

#endif
