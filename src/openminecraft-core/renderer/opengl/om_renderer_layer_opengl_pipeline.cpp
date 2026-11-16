#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_resource_set.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include <fmt/format.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <vector>

namespace openminecraft::renderer::opengl
{
OMRendererPipelineOpenGL::OMRendererPipelineOpenGL(OMRendererOpenGL *renderer)
    : common::OMRendererPipeline(renderer), logger("OMRendererPipelineOpenGL", this)
{
    this->rendererRef = renderer;
    this->gl = &renderer->gl;
}
OMRendererPipelineOpenGL::~OMRendererPipelineOpenGL()
{
    delete defaultSet;
    defaultSet = nullptr;

    if (program != 0)
    {
        gl->glDeleteProgram(program);
    }
}

void OMRendererPipelineOpenGL::bindInputName(std::string name)
{
    inputNames.push_back(name);
}

void OMRendererPipelineOpenGL::appendInput(common::OMRendererPipelineInputType t)
{
    inputTypes.push_back(t);
}
void OMRendererPipelineOpenGL::attachShader(std::shared_ptr<common::OMShader> shader)
{
    preshaders.push_back(shader);
}
void OMRendererPipelineOpenGL::vertexFormat(common::basics::OMVertexFormat format)
{
    this->format = format;
}
void OMRendererPipelineOpenGL::bindOutput(common::OMRendererRenderTarget *target)
{
    this->target = target;
}

static auto fromCommon(common::OMShaderType type) -> GLenum
{
    switch (type)
    {
    case common::Vertex:
    default:
        return GL_VERTEX_SHADER;
    case common::Fragment:
        return GL_FRAGMENT_SHADER;
    case common::Geometry:
        return GL_GEOMETRY_SHADER;
    case common::Compute:
        return GL_COMPUTE_SHADER;
    case common::TessControl:
        return GL_TESS_CONTROL_SHADER;
    case common::TessEvaluation:
        return GL_TESS_EVALUATION_SHADER;
    }
}
void OMRendererPipelineOpenGL::build()
{
    auto programbase = gl->glCreateProgram();
    std::vector<GLuint> subprogs;
    for (auto preshader : preshaders)
    {
        auto prog = gl->glCreateShader(fromCommon(preshader->typebase));
        auto source = reinterpret_cast<const GLchar *>(preshader->data.data());
        auto sl = (GLint)preshader->data.size();
        logger.info("compling {}", preshader->filename);
        gl->glShaderSource(prog, 1, &source, &sl);
        gl->glCompileShader(prog);

        GLint status;
        gl->glGetShaderiv(prog, GL_COMPILE_STATUS, &status);

        if (!status)
        {
            GLint logLength = 0;
            gl->glGetShaderiv(prog, GL_INFO_LOG_LENGTH, &logLength);
            std::vector<GLchar> log(static_cast<size_t>(std::max(logLength, 1)), '\0');
            gl->glGetShaderInfoLog(prog, static_cast<GLsizei>(log.size()), nullptr, log.data());
            const auto message = fmt::format("shader compile error in {}:\n{}", preshader->filename, log.data());
            logger.error("{}", message);
            throw std::runtime_error(message);
        }

        gl->glAttachShader(programbase, prog);
        subprogs.push_back(prog);
    }

    logger.info("linking");
    gl->glLinkProgram(programbase);

    GLint status;
    gl->glGetProgramiv(programbase, GL_LINK_STATUS, &status);
    if (!status)
    {
        std::vector<GLchar> log;
        GLint length;
        gl->glGetProgramiv(programbase, GL_INFO_LOG_LENGTH, &length);
        log.resize(length);
        gl->glGetProgramInfoLog(programbase, length, nullptr, log.data());
        const auto message = fmt::format("shader link error:\n{}", log.data());
        logger.error("{}", message);
        throw std::runtime_error(message);
    }

    for (auto s : subprogs)
    {
        gl->glDeleteShader(s);
    }

    program = programbase;

    for (int i = 0; i < inputNames.size(); ++i)
    {
        switch (inputTypes[i])
        {
        case common::ImageSampler:
        case common::UniformTexelBuffer:
            gl->glUseProgram(program);
            gl->glUniform1i(gl->glGetUniformLocation(program, inputNames[i].c_str()), i);
            break;
        case common::ShaderStorageBuffer:
            gl->glShaderStorageBlockBinding(
                program, gl->glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, inputNames[i].c_str()), i);
            break;
        case common::UniformBuffer:
            gl->glUniformBlockBinding(program, gl->glGetUniformBlockIndex(program, inputNames[i].c_str()), i);
            break;
        }
    }
}

void OMRendererPipelineOpenGL::setBlendFunc(common::OMReedererPipelineBlendState state)
{
    this->blendState.push_back(state);
}

void OMRendererPipelineOpenGL::bindInput(int idx, common::OMRendererBuffer *buff)
{
    defaultResourceSet()->bindInput(idx, buff);
}
void OMRendererPipelineOpenGL::bindInput(int idx, common::OMRendererTexture *texture)
{
    defaultResourceSet()->bindInput(idx, texture);
}

auto OMRendererPipelineOpenGL::createResourceSet() -> common::OMRendererResourceSet *
{
    return new OMRendererResourceSetOpenGL(rendererRef, this);
}
} // namespace openminecraft::renderer::opengl
