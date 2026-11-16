#ifndef OM_RENDERER_LAYER_OPENGL_RESOURCE_SET_HPP
#define OM_RENDERER_LAYER_OPENGL_RESOURCE_SET_HPP

#include "openminecraft/renderer/common/om_renderer_resource_set.hpp"
#include <vector>

namespace openminecraft::renderer::opengl
{
class OMRendererOpenGL;
class OMRendererPipelineOpenGL;

class OMRendererResourceSetOpenGL : public common::OMRendererResourceSet
{
  public:
    OMRendererResourceSetOpenGL(OMRendererOpenGL *renderer, OMRendererPipelineOpenGL *pipeline);

    void bindInput(int index, common::OMRendererBuffer *buffer) override;
    void bindInput(int index, common::OMRendererTexture *texture) override;
    void bindInputName(std::string name) override;

    auto inputs() const -> const std::vector<OMRendererObject *> &
    {
        return bound;
    }

  private:
    OMRendererPipelineOpenGL *glPipeline;
    std::vector<OMRendererObject *> bound;
};
} // namespace openminecraft::renderer::opengl

#endif
