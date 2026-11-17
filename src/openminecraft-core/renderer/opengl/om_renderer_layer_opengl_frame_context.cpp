#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_frame_context.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_task.hpp"

namespace openminecraft::renderer::opengl
{
void OMRendererFrameContextOpenGL::executeTasks(
    const std::vector<std::vector<common::OMRendererTask *>> &tasks)
{
    for (const auto &layer : tasks)
    {
        for (auto *task : layer)
        {
            if (!task->isDisabled())
            {
                reinterpret_cast<OMRendererTaskOpenGL *>(task)->execute();
            }
        }
    }
}
} // namespace openminecraft::renderer::opengl
