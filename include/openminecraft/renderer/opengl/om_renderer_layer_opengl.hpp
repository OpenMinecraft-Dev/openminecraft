#ifndef OM_RENDERER_LAYER_OPENGL_HPP
#define OM_RENDERER_LAYER_OPENGL_HPP

#include "openminecraft/renderer/om_renderer_layer.hpp"
namespace openminecraft::renderer::opengl
{
class OMRendererOpenGL : public OMRenderer
{
  public:
    OMRendererOpenGL(AppInfo info, void *window);
    ~OMRendererOpenGL();

    std::string driver() override;
    common::OMRendererBuffer *allocateBuffer(common::OMBufferUsage usage, uint64_t length) override;
    common::OMRendererTexture *allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type,
                                               common::OMTextureArrangement arr) override;
    common::OMRendererRenderTarget *createRenderTarget() override;
    common::OMRendererRenderTarget *getDefaultRenderTarget() override;
    common::OMRendererPipeline *createPipeline() override;
    common::OMRendererTask *createTask() override;
    void attachTask(common::OMRendererTask *task) override;
    glm::vec2 getExtent() const override;
};
} // namespace openminecraft::renderer::opengl

#endif
