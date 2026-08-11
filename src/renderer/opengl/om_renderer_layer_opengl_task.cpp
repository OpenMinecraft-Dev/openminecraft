#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_task.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_rendertarget.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"
#include <cstdint>
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

static auto convert(common::OMRendererPipelineBlendType t) -> GLenum
{
    switch (t)
    {
    default:
    case common::One:
        return GL_ONE;
    case common::Zero:
        return GL_ZERO;
    case common::Alpha:
        return GL_SRC_ALPHA;
    case common::OneMinusAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    }
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
        switch (glpipe->inputTypes[i])
        {
        case common::ImageSampler: {
            ops.push_back({ActiveTexture, static_cast<GLuint>(GL_TEXTURE0 + i)});
            auto tex = reinterpret_cast<OMRendererTextureOpenGL *>(glpipe->inputs[i]);
            switch (tex->type)
            {
            case common::Dim2:
                ops.push_back({BindTexture, GL_TEXTURE_2D, tex->texture});
                break;
            case common::Dim2Array:
                ops.push_back({BindTexture, GL_TEXTURE_2D_ARRAY, tex->texture});
                break;
            case common::Dim2Multisample:
                ops.push_back({BindTexture, GL_TEXTURE_2D_MULTISAMPLE, tex->texture});
                break;
            }

            break;
        }
        case common::UniformTexelBuffer:
            ops.push_back({ActiveTexture, static_cast<GLuint>(GL_TEXTURE0 + i)});
            ops.push_back(
                {BindTexture, GL_TEXTURE_BUFFER, reinterpret_cast<OMRendererBufferOpenGL *>(glpipe->inputs[i])->texel});

            break;
        case common::UniformBuffer:
            ops.push_back({BindBufferBase,
                           {GL_UNIFORM_BUFFER, static_cast<GLuint>(i),
                            reinterpret_cast<OMRendererBufferOpenGL *>(glpipe->inputs[i])->buffer}});
            break;
        case common::ShaderStorageBuffer:
            ops.push_back({BindBufferBase,
                           {GL_SHADER_STORAGE_BUFFER, static_cast<GLuint>(i),
                            reinterpret_cast<OMRendererBufferOpenGL *>(glpipe->inputs[i])->buffer}});
            break;
        }
    }

    auto s = glpipe->blendState;
    ops.push_back({glpipe->enableDepthTest ? Enable : Disable, GL_DEPTH_TEST});
    ops.push_back({glpipe->depthClamp ? Enable : Disable, GL_DEPTH_CLAMP});
    switch (glpipe->polygonMode)
    {
    default:
    case common::Fill:
        ops.push_back({PolygonMode, GL_FRONT_AND_BACK, GL_FILL});
        break;
    case common::Line:
        ops.push_back({PolygonMode, GL_FRONT_AND_BACK, GL_LINE});
        break;
    case common::Point:
        ops.push_back({PolygonMode, GL_FRONT_AND_BACK, GL_POINT});
        break;
    }
    ops.push_back({DepthMask, glpipe->enableDepthWrite});
    if (glpipe->enableReverseZ)
    {
        ops.push_back({DepthFunc, GL_GREATER});
    }
    else
    {
        ops.push_back({DepthFunc, GL_LESS});
    }
    ops.push_back({glpipe->enableBlend ? Enable : Disable, GL_BLEND});
    ops.push_back(
        {BlendFuncSeparate, convert(s.srcColor), convert(s.dstColor), convert(s.srcAlpha), convert(s.dstAlpha)});

    if (!isCleared)
    {
        ops.push_back({ClearDepth, {}, {}, depthClear});
        ops.push_back({ClearColor, {}, {}, colorClear.r, colorClear.g, colorClear.b, colorClear.a});
        ops.push_back({Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT});
        isCleared = true;
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

void OMRendererTaskOpenGL::bindVertexBufferInstanced(std::vector<common::OMRendererBuffer *> buffer, int off)
{
    int index = 0;
    for (auto pp : vtxFormat.parts)
    {
        reinterpret_cast<OMRendererBufferOpenGL *>(buffer[pp.binding])->bind();
        for (auto part : pp.parts)
        {
            auto offset = pp.isInstance ? off * pp.size : 0;

            auto p = fromCommon(std::get<common::basics::OMVertexPropType>(part));
            if (std::get<GLuint>(p) == GL_INT)
            {
                gl->glVertexAttribIPointer(index, p.first, p.second, pp.size,
                                           reinterpret_cast<void *>(std::get<int>(part) + offset));
            }
            else
            {
                gl->glVertexAttribPointer(index, p.first, p.second, GL_FALSE, pp.size,
                                          reinterpret_cast<void *>(std::get<int>(part) + offset));
            }
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

void OMRendererTaskOpenGL::bindVertexBuffer(std::vector<common::OMRendererBuffer *> buffer)
{
    int index = 0;
    for (auto pp : vtxFormat.parts)
    {
        reinterpret_cast<OMRendererBufferOpenGL *>(buffer[pp.binding])->bind();
        for (auto part : pp.parts)
        {
            auto p = fromCommon(std::get<common::basics::OMVertexPropType>(part));
            if (std::get<GLuint>(p) == GL_INT)
            {
                gl->glVertexAttribIPointer(index, p.first, p.second, pp.size,
                                           reinterpret_cast<void *>(std::get<int>(part)));
            }
            else
            {
                gl->glVertexAttribPointer(index, p.first, p.second, GL_FALSE, pp.size,
                                          reinterpret_cast<void *>(std::get<int>(part)));
            }
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

auto OMRendererTaskOpenGL::primitiveType() -> GLenum
{
    switch (pipeline->primitive)
    {
    default:
    case common::TriangleList:
        return GL_TRIANGLES;
    case common::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    case common::TriangleFan:
        return GL_TRIANGLE_FAN;
    case common::LineList:
        return GL_LINES;
    case common::LineStrip:
        return GL_LINE_STRIP;
    case common::PointList:
        return GL_POINTS;
    }
}
void OMRendererTaskOpenGL::drawIndirect(uint64_t begin, uint64_t count)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back({MultiDrawElementsIndirect,
                   {primitiveType(), GL_UNSIGNED_INT, static_cast<GLuint>(count), 5 * sizeof(uint32_t)},
                   {reinterpret_cast<void *>(begin * 5 * sizeof(uint32_t))}});
    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::bindTarget(common::OMRendererRenderTarget *target)
{
    ops.clear();
    this->framebuffer = reinterpret_cast<OMRendererRenderTargetOpenGL *>(target)->framebuffer;
    ops.push_back(
        {BindFramebuffer, GL_FRAMEBUFFER, reinterpret_cast<OMRendererRenderTargetOpenGL *>(target)->framebuffer});
    ops.push_back({Disable, GL_CULL_FACE});
    ops.push_back({Enable, GL_FRAMEBUFFER_SRGB});
    ops.push_back({Enable, GL_DEPTH_TEST});
    isCleared = false;
}
void OMRendererTaskOpenGL::draw(uint64_t vertexCount)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back({DrawArrays, primitiveType(), 0, static_cast<GLuint>(vertexCount)});
    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::drawInstance(uint64_t vertexCount, uint64_t instanceCount)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back({DrawArraysInstanced, primitiveType(), 0, static_cast<GLuint>(vertexCount),
                   static_cast<GLuint>(instanceCount)});
    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::drawInstance(uint64_t vertexCount, uint64_t instanceCount, uint64_t firstInstance)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back({DrawArraysInstanced, primitiveType(), 0, static_cast<GLuint>(vertexCount),
                   static_cast<GLuint>(instanceCount)});
    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::drawIndexed(uint64_t vertexCount)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back({DrawElements, {primitiveType(), static_cast<GLuint>(vertexCount), GL_UNSIGNED_INT}, {nullptr}});
    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::drawIndexedInstance(uint64_t vertexCount, uint64_t instanceCount)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back(
        {DrawElementsInstanced,
         {primitiveType(), static_cast<GLuint>(vertexCount), GL_UNSIGNED_INT, static_cast<GLuint>(instanceCount)},
         {nullptr}});
    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::drawIndexedInstance(uint64_t vertexCount, uint64_t instanceCount, uint64_t firstInstance)
{
    ops.push_back({BindVertexArray, vaos.back()});
    ops.push_back({UseProgram, program});
    ops.push_back(
        {DrawElementsInstanced,
         {primitiveType(), static_cast<GLuint>(vertexCount), GL_UNSIGNED_INT, static_cast<GLuint>(instanceCount)},
         {nullptr}});

    ops.push_back({BindVertexArray, 0});
    gl->glBindVertexArray(0);
}
void OMRendererTaskOpenGL::finish()
{
    gl->glBindVertexArray(0);
}

void OMRendererTaskOpenGL::resolveTo(common::OMRendererRenderTarget *target)
{
    auto result = reinterpret_cast<OMRendererRenderTargetOpenGL *>(target)->framebuffer;
    ops.push_back({BindFramebuffer, GL_READ_FRAMEBUFFER, framebuffer});
    ops.push_back({BindFramebuffer, GL_DRAW_FRAMEBUFFER, result});
    auto siz = target->fetchSize();
    GLuint wid = siz.x;
    GLuint hei = siz.y;
    ops.push_back({BlitFramebuffer, 0, 0, wid, hei, 0, 0, wid, hei, GL_COLOR_BUFFER_BIT, GL_NEAREST});
    ops.push_back({BindFramebuffer, GL_READ_FRAMEBUFFER, 0});
    ops.push_back({BindFramebuffer, GL_DRAW_FRAMEBUFFER, 0});
}

int a = 0;
void OMRendererTaskOpenGL::execute()
{
    for (auto &op : ops)
    {
        switch (op.type)
        {
        case BlitFramebuffer:
            gl->glBlitFramebuffer(op.args[0], op.args[1], op.args[2], op.args[3], op.args[4], op.args[5], op.args[6],
                                  op.args[7], op.args[8], op.args[9]);
            break;
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
        case DrawElementsInstancedBaseInstance:
            gl->glDrawElementsInstancedBaseInstance(op.args[0], op.args[1], op.args[2], op.ptrArgs[0], op.args[3],
                                                    op.args[4]);
            break;
        case MultiDrawElementsIndirect:
            gl->glMultiDrawElementsIndirect(op.args[0], op.args[1], op.ptrArgs[0], op.args[2], op.args[3]);
            break;
        case BindBuffer:
            gl->glBindBuffer(op.args[0], op.args[1]);
            break;
        case BlendFunc:
            gl->glBlendFunc(op.args[0], op.args[1]);
            break;
        case DepthFunc:
            gl->glDepthFunc(op.args[0]);
            break;
        case BlendFuncSeparate:
            gl->glBlendFuncSeparate(op.args[0], op.args[1], op.args[2], op.args[3]);
            break;
        case DepthMask:
            gl->glDepthMask(op.args[0]);
            break;
        case ClearDepth:
            gl->glClearDepth(op.floatArgs[0]);
            break;
        case ClearBufferfv:
            gl->glClearBufferfv(GL_DEPTH, 1, &op.floatArgs[0]);
            break;
        case ClearColor:
            gl->glClearColor(op.floatArgs[0], op.floatArgs[1], op.floatArgs[2], op.floatArgs[3]);
            break;
        case DrawArraysInstanced:
            gl->glDrawArraysInstanced(op.args[0], op.args[1], op.args[2], op.args[3]);
            break;
        case DrawArrays:
            gl->glDrawArrays(op.args[0], op.args[1], op.args[2]);
            break;
        case PolygonMode:
            gl->glPolygonMode(op.args[0], op.args[1]);
            break;
        }
    }
}
} // namespace openminecraft::renderer::opengl
