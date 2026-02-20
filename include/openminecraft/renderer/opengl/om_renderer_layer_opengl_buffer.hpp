#ifndef OM_RENDERER_LAYER_OPENGL_BUFFER_HPP
#define OM_RENDERER_LAYER_OPENGL_BUFFER_HPP

#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include <cstdint>
namespace openminecraft::renderer::opengl
{
class OMRendererBufferOpenGL : public common::OMRendererBuffer
{
  public:
    OMRendererBufferOpenGL(common::OMBufferUsage usage, uint64_t length, OMRendererOpenGL *);
    ~OMRendererBufferOpenGL() override;

    void updateData(void *src) override;
    void bind();
    void unbind();
    GLuint buffer;

  private:
    OMRendererOpenGL *renderer;
    uint64_t length;
    common::OMBufferUsage usage;
};

} // namespace openminecraft::renderer::opengl

#endif
