#ifndef OM_RENDERER_LAYER_OPENGL_TEXTURE_HPP
#define OM_RENDERER_LAYER_OPENGL_TEXTURE_HPP

#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"

namespace openminecraft::renderer::opengl
{
class OMRendererTextureOpenGL : public common::OMRendererTexture
{
  public:
    OMRendererTextureOpenGL(uint64_t width, uint64_t height, uint64_t mipmap, common::OMTextureType type,
                            common::OMTextureArrangement arr, OMRendererOpenGL *renderer);
    ~OMRendererTextureOpenGL() override;

    void updateData(void *) override;
    void updateDataPart(void *p, uint64_t x, uint64_t y, uint64_t w, uint64_t h) override;
    void setupSampler() override;
    GLuint texture;

  private:
    uint64_t mipmap;
    OMRendererOpenGLFuncs *gl;
};
} // namespace openminecraft::renderer::opengl

#endif
