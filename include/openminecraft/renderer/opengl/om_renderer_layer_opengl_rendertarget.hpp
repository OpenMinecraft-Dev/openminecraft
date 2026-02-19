#ifndef OM_RENDERER_LAYER_OPENGL_RENDERTARGET_HPP
#define OM_RENDERER_LAYER_OPENGL_RENDERTARGET_HPP

#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include <vector>

namespace openminecraft::renderer::opengl
{
class OMRendererRenderTargetOpenGL : public common::OMRendererRenderTarget
{
  public:
    OMRendererRenderTargetOpenGL(OMRendererOpenGL *renderer);
    ~OMRendererRenderTargetOpenGL() override;

    void attachTarget(common::OMRendererTexture *texture) override;
    void replaceTarget(int idx, common::OMRendererTexture *texture) override;
    void rebuild() override;
    glm::vec2 fetchSize() override;
    void build() override;

  private:
    OMRendererOpenGLFuncs *gl;
    GLuint framebuffer;
    std::vector<common::OMRendererTexture *> textures;
};
} // namespace openminecraft::renderer::opengl

#endif
