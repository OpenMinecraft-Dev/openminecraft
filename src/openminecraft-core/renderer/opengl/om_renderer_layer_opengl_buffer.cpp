#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/mem/om_mem_record.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include <algorithm>
#include <cstring>
#include <vector>

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

static auto convertFrom(common::basics::OMVertexPropType type) -> GLenum
{
    switch (type)
    {
    default:
    case common::basics::Float:
        return GL_R32F;
    case common::basics::Vec2f:
        return GL_RG32F;
    case common::basics::Vec3f:
        return GL_RGB32F;
    case common::basics::Vec4f:
        return GL_RGBA32F;
    case common::basics::Integer:
        return GL_R32I;
    case common::basics::Vec2i:
        return GL_RG32I;
    case common::basics::Vec3i:
        return GL_RGB32I;
    case common::basics::Vec4i:
        return GL_RGBA32I;
    case common::basics::UInteger:
        return GL_R32UI;
    case common::basics::Vec2ui:
        return GL_RG32UI;
    case common::basics::Vec3ui:
        return GL_RGB32UI;
    case common::basics::Vec4ui:
        return GL_RGBA32UI;
    }
}

OMRendererBufferOpenGL::OMRendererBufferOpenGL(common::OMBufferUsage usage, uint64_t length, OMRendererOpenGL *renderer)
    : common::OMRendererBuffer(usage, length, renderer), renderer(renderer), length(length), usage(usage)
{
    renderer->gl.glGenBuffers(1, &buffer);

    if (usage == common::UniformTexel)
    {
        renderer->gl.glGenTextures(1, &texel);
    }

    mem::castorice::rec({mem::castorice::Allocation, nullptr, static_cast<size_t>(length), "opengl"});

    updateData(nullptr);
}

OMRendererBufferOpenGL::~OMRendererBufferOpenGL()
{
    if (usage == common::UniformTexel && texel != 0)
    {
        renderer->gl.glDeleteTextures(1, &texel);
    }
    renderer->gl.glDeleteBuffers(1, &buffer);
    mem::castorice::rec({mem::castorice::Free, nullptr, static_cast<size_t>(length), "opengl"});
}
void OMRendererBufferOpenGL::updateData(void *src)
{
    bind();

    if (length > 0)
    {
        renderer->gl.glBufferData(convertFrom(usage), length, src,
                                  (usage == common::VertexData || usage == common::VertexIndex) ? GL_STATIC_DRAW
                                                                                                : GL_DYNAMIC_DRAW);
    }

    if (usage == common::UniformTexel && texel != 0)
    {
        renderer->gl.glBindTexture(GL_TEXTURE_BUFFER, texel);
        renderer->gl.glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffer);
    }
    unbind();
}
void OMRendererBufferOpenGL::moveDataPart(uint64_t target, uint64_t original, uint64_t length)
{
    if (length == 0 || target == original)
    {
        return;
    }

    bind();

    const uint64_t lo = std::min(target, original);
    const uint64_t hi = std::max(target + length, original + length);

    void *data = renderer->gl.glMapBufferRange(convertFrom(usage), static_cast<GLintptr>(lo),
                                               static_cast<GLsizeiptr>(hi - lo), GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

    if (data != nullptr)
    {
        auto *base = static_cast<uint8_t *>(data);
        std::memmove(base + (target - lo), base + (original - lo), static_cast<size_t>(length));
        renderer->gl.glUnmapBuffer(convertFrom(usage));
    }
    else
    {
        // 映射失败时退化为 读回 + 写回
        std::vector<uint8_t> temp(static_cast<size_t>(length));
        renderer->gl.glGetBufferSubData(convertFrom(usage), static_cast<GLintptr>(original),
                                        static_cast<GLsizeiptr>(length), temp.data());
        renderer->gl.glBufferSubData(convertFrom(usage), static_cast<GLintptr>(target), static_cast<GLsizeiptr>(length),
                                     temp.data());
    }

    if (usage == common::UniformTexel && texel != 0)
    {
        renderer->gl.glBindTexture(GL_TEXTURE_BUFFER, texel);
        renderer->gl.glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffer);
    }
    unbind();
}
void OMRendererBufferOpenGL::updateDataPart(void *src, uint64_t offset, uint64_t length)
{
    bind();

    if (length > 0)
    {
        void *data = renderer->gl.glMapBufferRange(convertFrom(usage), static_cast<GLintptr>(offset),
                                                   static_cast<GLsizeiptr>(length),
                                                   GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

        if (data != nullptr)
        {
            std::memcpy(data, src, static_cast<size_t>(length));
            renderer->gl.glUnmapBuffer(convertFrom(usage));
        }
        else
        {
            renderer->gl.glBufferSubData(convertFrom(usage), static_cast<GLintptr>(offset),
                                         static_cast<GLsizeiptr>(length), src);
        }
    }

    if (usage == common::UniformTexel && texel != 0)
    {
        renderer->gl.glBindTexture(GL_TEXTURE_BUFFER, texel);
        renderer->gl.glTexBuffer(GL_TEXTURE_BUFFER, GL_R32F, buffer);
    }
    unbind();
}
auto OMRendererBufferOpenGL::fetchData() -> std::shared_ptr<void>
{
    std::shared_ptr<void> result(std::malloc(length), [](void *p) { std::free(p); });

    bind();
    renderer->gl.glGetBufferSubData(convertFrom(usage), 0, length, result.get());
    unbind();

    return result;
}

void OMRendererBufferOpenGL::copyTo(OMRendererBuffer *buf)
{
    void *temp = malloc(length);

    bind();
    renderer->gl.glGetBufferSubData(convertFrom(usage), 0, length, temp);
    unbind();

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
