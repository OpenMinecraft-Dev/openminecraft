#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"

namespace openminecraft::renderer::opengl
{
static auto convertFrom(common::OMBufferUsage u) -> GLenum
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
    case common::Indirect:
        return GL_DRAW_INDIRECT_BUFFER;
    case common::ShaderStorage:
        return GL_SHADER_STORAGE_BUFFER;
    case common::UniformTexel:
        return GL_TEXTURE_BUFFER;
    default:
        return GL_COPY_WRITE_BUFFER;
    }
}

OMRendererBufferOpenGL::OMRendererBufferOpenGL(common::OMBufferUsage usage, uint64_t length, OMRendererOpenGL *renderer)
    : usage(usage), length(length), renderer(renderer), common::OMRendererBuffer(usage, length, renderer)
{
    renderer->gl.glGenBuffers(1, &buffer);
    mem::castorice::rec({mem::castorice::Allocation, nullptr, static_cast<size_t>(length), "opengl"});

    updateData(nullptr);

    if (usage == common::UniformTexel)
    {
        renderer->gl.glGenTextures(1, &texel);
        renderer->gl.glBindTexture(GL_TEXTURE_BUFFER, texel);
        renderer->gl.glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffer);
    }
}

OMRendererBufferOpenGL::~OMRendererBufferOpenGL()
{
    if (usage == common::UniformTexel)
    {
        renderer->gl.glDeleteTextures(1, &texel);
    }
    renderer->gl.glDeleteBuffers(1, &buffer);
    mem::castorice::rec({mem::castorice::Free, nullptr, static_cast<size_t>(length), "opengl"});
}
void OMRendererBufferOpenGL::updateData(void *src)
{
    bind();
    renderer->gl.glBufferData(convertFrom(usage), length, src,
                              (usage == common::VertexData || usage == common::VertexIndex) ? GL_STATIC_DRAW
                                                                                            : GL_DYNAMIC_DRAW);
}
void OMRendererBufferOpenGL::updateDataPart(void *src, uint64_t offset, uint64_t length)
{
    bind();
    renderer->gl.glBufferSubData(convertFrom(usage), offset, length, src);
}
void OMRendererBufferOpenGL::copyTo(OMRendererBuffer *buf)
{
    void *temp = malloc(length);

    bind();
    renderer->gl.glGetBufferSubData(convertFrom(usage), 0, length, temp);

    buf->updateDataPart(temp, 0, length);
    free(temp);
}
void OMRendererBufferOpenGL::bind()
{
    renderer->gl.glBindBuffer(convertFrom(usage), buffer);
}

void OMRendererBufferOpenGL::unbind()
{
    renderer->gl.glBindBuffer(convertFrom(usage), 0);
}
} // namespace openminecraft::renderer::opengl
