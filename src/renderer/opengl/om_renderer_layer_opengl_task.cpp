#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_task.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_rendertarget.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"
#include <utility>

namespace openminecraft::renderer::opengl
{
OMRendererTaskOpenGL::OMRendererTaskOpenGL(OMRendererOpenGL *renderer) : common::OMRendererTask(renderer)
{
    this->gl = &renderer->gl;
    gl->glGenVertexArrays(1, &vertexArrayObject);
    gl->glBindVertexArray(vertexArrayObject);
}
OMRendererTaskOpenGL::~OMRendererTaskOpenGL()
{
}

void OMRendererTaskOpenGL::bindPipeline(common::OMRendererPipeline *pipeline)
{
    auto glpipe = reinterpret_cast<OMRendererPipelineOpenGL *>(pipeline);
    this->program = glpipe->program;
    this->framebuffer = reinterpret_cast<OMRendererRenderTargetOpenGL *>(glpipe->target)->framebuffer;
    vtxFormat = glpipe->format;
    this->pipeline = glpipe;

    for (int i = 0; i < glpipe->inputTypes.size(); i++)
    {
        if (glpipe->inputTypes[i] == common::UniformBuffer)
        {
            gl->glUniformBlockBinding(this->program,
                                      reinterpret_cast<OMRendererBufferOpenGL *>(glpipe->inputs[i])->buffer, i);
        }
    }
}

static std::pair<int, GLuint> fromCommon(common::basics::OMVertexPropType t)
{
    switch (t)
    {
    case common::basics::Float:
        return std::make_pair(1, GL_FLOAT);
    case common::basics::Vec2f:
        return std::make_pair(2, GL_FLOAT);
    case common::basics::Vec3f:
    default:
        return std::make_pair(3, GL_FLOAT);
    case common::basics::Vec4f:
        return std::make_pair(4, GL_FLOAT);
    case common::basics::Integer:
        return std::make_pair(1, GL_INT);
    case common::basics::Vec2i:
        return std::make_pair(2, GL_INT);
    case common::basics::Vec3i:
        return std::make_pair(3, GL_INT);
    case common::basics::Vec4i:
        return std::make_pair(4, GL_INT);
    case common::basics::Double:
        return std::make_pair(1, GL_DOUBLE);
    case common::basics::Vec2d:
        return std::make_pair(2, GL_DOUBLE);
    case common::basics::Vec3d:
        return std::make_pair(3, GL_DOUBLE);
    case common::basics::Vec4d:
        return std::make_pair(4, GL_DOUBLE);
    }
}

void OMRendererTaskOpenGL::bindVertexBuffer(std::vector<common::OMRendererBuffer *> buffer)
{
    int index = 0;
    for (auto pp : vtxFormat.parts)
    {
        reinterpret_cast<OMRendererBufferOpenGL *>(buffer[pp.binding])->bind();
        for (auto part : pp.parts)
        {
            auto p = fromCommon(std::get<common::basics::OMVertexPropType>(part));
            gl->glVertexAttribPointer(index, 2, GL_FLOAT, GL_FALSE, pp.size,
                                      reinterpret_cast<void *>(std::get<int>(part)));
            gl->glEnableVertexAttribArray(index);
            index++;
        }
        reinterpret_cast<OMRendererBufferOpenGL *>(buffer[pp.binding])->unbind();
    }
}
void OMRendererTaskOpenGL::bindIndexBuffer(common::OMRendererBuffer *buffer)
{
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, reinterpret_cast<OMRendererBufferOpenGL *>(buffer)->buffer);
}

// we don't need to do anything, render target is fixed in the pipeline
void OMRendererTaskOpenGL::bindTarget(common::OMRendererRenderTarget *target)
{
}
void OMRendererTaskOpenGL::draw(uint64_t vertexCount)
{
    this->vtxCount = vertexCount;
}
void OMRendererTaskOpenGL::finish()
{
    gl->glBindVertexArray(0);
}

void OMRendererTaskOpenGL::execute()
{
    gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    gl->glDisable(GL_CULL_FACE);
    gl->glBindVertexArray(vertexArrayObject);
    gl->glUseProgram(program);

    for (int i = 0; i < pipeline->inputTypes.size(); i++)
    {
        if (pipeline->inputTypes[i] == common::UniformBuffer)
        {
            gl->glBindBufferBase(GL_UNIFORM_BUFFER, i,
                                 reinterpret_cast<OMRendererBufferOpenGL *>(pipeline->inputs[i])->buffer);
        }

        else if (pipeline->inputTypes[i] == common::ImageSampler)
        {
            gl->glActiveTexture(GL_TEXTURE0 + i);
            gl->glBindTexture(GL_TEXTURE_2D, reinterpret_cast<OMRendererTextureOpenGL *>(pipeline->inputs[i])->texture);
        }
    }

    gl->glDrawElements(GL_TRIANGLES, vtxCount, GL_UNSIGNED_INT, 0);

    gl->glBindVertexArray(0);
}
} // namespace openminecraft::renderer::opengl
