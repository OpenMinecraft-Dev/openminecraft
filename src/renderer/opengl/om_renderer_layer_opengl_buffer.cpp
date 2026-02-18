#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"

namespace openminecraft::renderer::opengl
{
static GLenum convertFrom(common::OMBufferUsage u)
{
    switch (u)
    {
    case common::VertexData:
    case common::InstanceData:
        return GL_ARRAY_BUFFER;
    case common::VertexIndex:
        return GL_ELEMENT_ARRAY_BUFFER;
    case common::Uniform:
        return GL_UNIFORM_BUFFER;
    default:
        return GL_COPY_WRITE_BUFFER;
    }
}

OMRendererBufferOpenGL::OMRendererBufferOpenGL(common::OMBufferUsage usage, uint64_t length, OMRendererOpenGL *renderer)
    : usage(usage), length(length), renderer(renderer), common::OMRendererBuffer(usage, length, renderer)
{
    renderer->gl.glGenBuffers(1, &buffer);
    mem::castorice::rec({mem::castorice::Allocation, nullptr, static_cast<size_t>(length), "opengl"});
}

OMRendererBufferOpenGL::~OMRendererBufferOpenGL()
{
    renderer->gl.glDeleteBuffers(1, &buffer);
    mem::castorice::rec({mem::castorice::Free, nullptr, static_cast<size_t>(length), "opengl"});
}
void OMRendererBufferOpenGL::updateData(void *src)
{

    renderer->gl.glBindBuffer(convertFrom(usage), buffer);
    renderer->gl.glBufferData(convertFrom(usage), length, src, GL_STATIC_DRAW);
}
} // namespace openminecraft::renderer::opengl
