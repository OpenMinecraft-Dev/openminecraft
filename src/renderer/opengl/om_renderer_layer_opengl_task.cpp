#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_task.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_rendertarget.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"
#include <cstdint>
#include <iostream>
#include <utility>

namespace openminecraft::renderer::opengl
{
OMRendererTaskOpenGL::OMRendererTaskOpenGL(OMRendererOpenGL *renderer) : common::OMRendererTask(renderer)
{
    this->gl = &renderer->gl;
}
OMRendererTaskOpenGL::~OMRendererTaskOpenGL()
{
    gl->glDeleteVertexArrays(vaos.size(), vaos.data());
}

void OMRendererTaskOpenGL::bindPipeline(common::OMRendererPipeline *pipeline)
{
    auto glpipe = reinterpret_cast<OMRendererPipelineOpenGL *>(pipeline);
    this->program = glpipe->program;
    vtxFormat = glpipe->format;
    this->pipeline = glpipe;

    GLuint vao;
    gl->glGenVertexArrays(1, &vao);
    gl->glBindVertexArray(vao);
    vaos.push_back(vao);

    for (int i = 0; i < glpipe->inputTypes.size(); i++)
    {
        auto obj = glpipe->inputs[i];
        if (obj->objType() == DataBuffer)
        {
            ops.push_back({BindBufferBase,
                           {GL_UNIFORM_BUFFER, static_cast<GLuint>(i),
                            reinterpret_cast<OMRendererBufferOpenGL *>(glpipe->inputs[i])->buffer}});
        }
        else if (obj->objType() == Texture)
        {
            ops.push_back({ActiveTexture, static_cast<GLuint>(GL_TEXTURE0 + i)});
            ops.push_back(
                {BindTexture, GL_TEXTURE_2D, reinterpret_cast<OMRendererTextureOpenGL *>(glpipe->inputs[i])->texture});
        }
    }
}

static auto fromCommon(common::basics::OMVertexPropType t) -> std::pair<int, GLuint>
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
            gl->glVertexAttribPointer(index, p.first, p.second, GL_FALSE, pp.size,
                                      reinterpret_cast<void *>(std::get<int>(part)));
            gl->glEnableVertexAttribArray(index);
            if (pp.isInstance)
            {
                gl->glVertexAttribDivisor(index, 1);
            }
            index++;
        }
        reinterpret_cast<OMRendererBufferOpenGL *>(buffer[pp.binding])->unbind();
    }
}
void OMRendererTaskOpenGL::bindIndexBuffer(common::OMRendererBuffer *buffer)
{
    gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, reinterpret_cast<OMRendererBufferOpenGL *>(buffer)->buffer);
}
void OMRendererTaskOpenGL::bindIndirectBuffer(common::OMRendererBuffer *buffer)
{
    ops.push_back({BindBuffer, GL_DRAW_INDIRECT_BUFFER, reinterpret_cast<OMRendererBufferOpenGL *>(buffer)->buffer});
}
void OMRendererTaskOpenGL::drawIndirect(uint64_t begin, uint64_t count)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back({MultiDrawElementsIndirect,
                   {GL_TRIANGLES, GL_UNSIGNED_INT, static_cast<GLuint>(count), 5 * sizeof(uint32_t)},
                   {reinterpret_cast<void *>(begin * 5 * sizeof(uint32_t))}});
    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::clear()
{
    ops.push_back({Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT});
}
void OMRendererTaskOpenGL::bindTarget(common::OMRendererRenderTarget *target)
{
    this->framebuffer = reinterpret_cast<OMRendererRenderTargetOpenGL *>(target)->framebuffer;
    ops.push_back(
        {BindFramebuffer, GL_FRAMEBUFFER, reinterpret_cast<OMRendererRenderTargetOpenGL *>(target)->framebuffer});
    ops.push_back({Disable, GL_CULL_FACE});
    ops.push_back({Enable, GL_FRAMEBUFFER_SRGB});
}
void OMRendererTaskOpenGL::draw(uint64_t vertexCount)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back({DrawElements, {GL_TRIANGLES, static_cast<GLuint>(vertexCount), GL_UNSIGNED_INT}, {nullptr}});
    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::drawInstance(uint64_t vertexCount, uint64_t instanceCount)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back(
        {DrawElementsInstanced,
         {GL_TRIANGLES, static_cast<GLuint>(vertexCount), GL_UNSIGNED_INT, static_cast<GLuint>(instanceCount)},
         {nullptr}});
    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::finish()
{
    gl->glBindVertexArray(0);
}

int a = 0;
void OMRendererTaskOpenGL::execute()
{
    for (auto &op : ops)
    {
        switch (op.type)
        {
        case BindFramebuffer:
            gl->glBindFramebuffer(op.args[0], op.args[1]);
            break;
        case Clear:
            gl->glClear(op.args[0]);
            break;
        case Disable:
            gl->glDisable(op.args[0]);
            break;
        case Enable:
            gl->glEnable(op.args[0]);
            break;
        case BindBufferBase:
            gl->glBindBufferBase(op.args[0], op.args[1], op.args[2]);
            break;
        case ActiveTexture:
            gl->glActiveTexture(op.args[0]);
            break;
        case BindTexture:
            gl->glBindTexture(op.args[0], op.args[1]);
            break;
        case BindVertexArray:
            gl->glBindVertexArray(op.args[0]);
            break;
        case UseProgram:
            gl->glUseProgram(op.args[0]);
            break;
        case DrawElements:
            gl->glDrawElements(op.args[0], op.args[1], op.args[2], op.ptrArgs[0]);
            break;
        case DrawElementsInstanced:
            gl->glDrawElementsInstanced(op.args[0], op.args[1], op.args[2], op.ptrArgs[0], op.args[3]);
            break;
        case MultiDrawElementsIndirect:
            gl->glMultiDrawElementsIndirect(op.args[0], op.args[1], op.ptrArgs[0], op.args[2], op.args[3]);
            break;
        case BindBuffer:
            gl->glBindBuffer(op.args[0], op.args[1]);
            break;
        }
    }
}
} // namespace openminecraft::renderer::opengl
