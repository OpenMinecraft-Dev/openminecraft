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
static auto convert(common::OMRendererPipelineBlendOp o) -> GLenum
{
    switch (o)
    {
    default:
    case common::Add:
        return GL_FUNC_ADD;
    case common::Subtract:
        return GL_FUNC_SUBTRACT;
    case common::ReverseSubstract:
        return GL_FUNC_REVERSE_SUBTRACT;
    case common::Min:
        return GL_MIN;
    case common::Max:
        return GL_MAX;
    }
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
    case common::SrcAlpha:
        return GL_SRC_ALPHA;
    case common::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
    case common::DstAlpha:
        return GL_DST_ALPHA;
    case common::OneMinusDstAlpha:
        return GL_ONE_MINUS_DST_ALPHA;
    case common::SrcColor:
        return GL_SRC_COLOR;
    case common::OneMinusSrcColor:
        return GL_ONE_MINUS_SRC_COLOR;
    case common::DstColor:
        return GL_DST_COLOR;
    case common::OneMinusDstColor:
        return GL_ONE_MINUS_DST_COLOR;
    case common::ConstantColor:
        return GL_CONSTANT_COLOR;
    case common::OneMinusConstantColor:
        return GL_ONE_MINUS_CONSTANT_COLOR;
    case common::ConstantAlpha:
        return GL_CONSTANT_ALPHA;
    case common::OneMinusConstantAlpha:
        return GL_ONE_MINUS_CONSTANT_ALPHA;
    case common::SrcAlphaSaturate:
        return GL_SRC_ALPHA_SATURATE;
    case common::Src1Color:
        return GL_SRC1_COLOR;
    case common::OneMinusSrc1Color:
        return GL_ONE_MINUS_SRC1_COLOR;
    case common::Src1Alpha:
        return GL_SRC1_ALPHA;
    case common::OneMinusSrc1Alpha:
        return GL_ONE_MINUS_SRC1_ALPHA;
    }
}

static auto convert(common::OMRendererPipelineBlendLogicOp o) -> GLenum
{
    switch (o)
    {
    case common::Clear:
        return GL_CLEAR;
    case common::And:
        return GL_AND;
    case common::AndReverse:
        return GL_AND_REVERSE;
    case common::Copy:
        return GL_COPY;
    case common::AndInverted:
        return GL_AND_INVERTED;
    default:
    case common::NoOp:
        return GL_NOOP;
    case common::Xor:
        return GL_XOR;
    case common::Or:
        return GL_OR;
    case common::Nor:
        return GL_NOR;
    case common::Equivalent:
        return GL_EQUIV;
    case common::Invert:
        return GL_INVERT;
    case common::OrReverse:
        return GL_OR_REVERSE;
    case common::CopyInverted:
        return GL_COPY_INVERTED;
    case common::OrInverted:
        return GL_OR_INVERTED;
    case common::Nand:
        return GL_NAND;
    case common::Set:
        return GL_SET;
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
    switch (glpipe->cullMode)
    {
    default:
    case common::None:
        ops.push_back({Disable, GL_CULL_FACE});
        break;
    case common::Front:
        ops.push_back({Enable, GL_CULL_FACE});
        ops.push_back({CullFace, GL_FRONT});
        break;
    case common::Back:
        ops.push_back({Enable, GL_CULL_FACE});
        ops.push_back({CullFace, GL_BACK});
        break;
    case common::FrontAndBack:
        ops.push_back({Enable, GL_CULL_FACE});
        ops.push_back({CullFace, GL_FRONT_AND_BACK});
        break;
    }
    if (glpipe->cullClockwise)
    {
        ops.push_back({FrontFace, GL_CW});
    }
    else
    {
        ops.push_back({FrontFace, GL_CCW});
    }
    ops.push_back({glpipe->enableDepthBias ? Enable : Disable, GL_POLYGON_OFFSET_FILL});
    ops.push_back({PolygonOffset, {}, {}, glpipe->depthBiasSlope, glpipe->depthBiasConstant});
    ops.push_back({LineWidth, {}, {}, glpipe->lineWidth});
    ops.push_back({DepthMask, glpipe->enableDepthWrite});
    ops.push_back({glpipe->enableDepthTest ? Enable : Disable, GL_DEPTH_TEST});
    switch (glpipe->depthOperator)
    {
    case common::Greater:
        ops.push_back({DepthFunc, GL_GREATER});
        break;
    case common::Less:
        ops.push_back({DepthFunc, GL_LESS});
        break;
    case common::GreaterOrEqual:
        ops.push_back({DepthFunc, GL_GEQUAL});
        break;
    case common::LessOrEqual:
        ops.push_back({DepthFunc, GL_LEQUAL});
        break;
    case common::Equal:
        ops.push_back({DepthFunc, GL_EQUAL});
        break;
    case common::NotEqual:
        ops.push_back({DepthFunc, GL_NOTEQUAL});
        break;
    default:
    case common::Always:
        ops.push_back({DepthFunc, GL_ALWAYS});
        break;
    case common::Never:
        ops.push_back({DepthFunc, GL_NEVER});
        break;
    }
    ops.push_back({glpipe->enableBlend ? Enable : Disable, GL_BLEND});
    ops.push_back(
        {BlendFuncSeparate, convert(s.srcColor), convert(s.dstColor), convert(s.srcAlpha), convert(s.dstAlpha)});
    ops.push_back({BlendEquationSeparate, convert(glpipe->blendOperatorColor), convert(glpipe->blendOperatorAlpha)});
    ops.push_back(
        {BlendColor,
         {},
         {},
         {glpipe->blendConstant.r, glpipe->blendConstant.g, glpipe->blendConstant.b, glpipe->blendConstant.a}});

    ops.push_back({glpipe->enableMultisampleShading ? Enable : Disable, GL_SAMPLE_SHADING});
    ops.push_back({glpipe->enablePrimitiveRestart ? Enable : Disable, GL_PRIMITIVE_RESTART_FIXED_INDEX});
    ops.push_back({glpipe->enableBlendLogicOp ? Enable : Disable, GL_COLOR_LOGIC_OP});
    ops.push_back({LogicOp, convert(glpipe->blendLogicOperator)});

    if (!isCleared)
    {
        ops.push_back({ClearDepth, {}, {}, depthClear});
        ops.push_back({ClearColor, {}, {}, colorClear.r, colorClear.g, colorClear.b, colorClear.a});
        if (needClearDepth)
        {
            ops.push_back({Clear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT});
        }
        else
        {
            ops.push_back({Clear, GL_COLOR_BUFFER_BIT});
        }
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
    auto tgt = reinterpret_cast<OMRendererRenderTargetOpenGL *>(target);
    this->framebuffer = tgt->framebuffer;
    ops.push_back({BindFramebuffer, GL_FRAMEBUFFER, tgt->framebuffer});
    ops.push_back({Enable, GL_FRAMEBUFFER_SRGB});
    isCleared = false;
    needClearDepth = target->clearDepth;

    auto siz = tgt->fetchSize();

    ops.push_back({Viewport, 0, 0, static_cast<GLuint>(siz.x), static_cast<GLuint>(siz.y)});
    ops.push_back({Scissor, 0, 0, static_cast<GLuint>(siz.x), static_cast<GLuint>(siz.y)});
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
        case CullFace:
            gl->glCullFace(op.args[0]);
            break;
        case FrontFace:
            gl->glFrontFace(op.args[0]);
            break;
        case LineWidth:
            gl->glLineWidth(op.floatArgs[0]);
            break;
        case PolygonOffset:
            gl->glPolygonOffset(op.floatArgs[0], op.floatArgs[1]);
            break;
        case BlendColor:
            gl->glBlendColor(op.floatArgs[0], op.floatArgs[1], op.floatArgs[2], op.floatArgs[3]);
            break;
        case BlendEquationSeparate:
            gl->glBlendEquationSeparate(op.args[0], op.args[1]);
            break;
        case LogicOp:
            gl->glLogicOp(op.args[0]);
            break;
        case Scissor:
            gl->glScissor(op.args[0], op.args[1], op.args[2], op.args[3]);
            break;
        case Viewport:
            gl->glViewport(op.args[0], op.args[1], op.args[2], op.args[3]);
            break;
        }
    }
}
} // namespace openminecraft::renderer::opengl
