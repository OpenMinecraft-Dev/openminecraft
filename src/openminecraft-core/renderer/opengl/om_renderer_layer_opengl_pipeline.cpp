#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include <fmt/format.h>
#include <stdexcept>
#include <vector>

namespace openminecraft::renderer::opengl
{
OMRendererPipelineOpenGL::OMRendererPipelineOpenGL(OMRendererOpenGL *renderer)
    : common::OMRendererPipeline(renderer), logger("OMRendererPipelineOpenGL", this)
{
    this->gl = &renderer->gl;
}
OMRendererPipelineOpenGL::~OMRendererPipelineOpenGL()
{
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
    inputs.resize(inputTypes.size());
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
    for (auto pp : preshaders)
    {
        auto prog = gl->glCreateShader(fromCommon(pp->typebase));
        auto ss = reinterpret_cast<const GLchar *>(pp->data.data());
        auto sl = (GLint)pp->data.size();
        logger.info("compling {}", pp->filename);
        gl->glShaderSource(prog, 1, &ss, &sl);
        gl->glCompileShader(prog);

        GLint status;
        gl->glGetShaderiv(prog, GL_COMPILE_STATUS, &status);

        if (!status)
        {
            GLsizei l;
            std::vector<GLchar> log = {};
            gl->glGetProgramInfoLog(prog, 0, &l, log.data());
            log.resize(l);
            gl->glGetProgramInfoLog(prog, 1024, nullptr, log.data());
            throw std::runtime_error(fmt::format("compile error: {}", log.data()));
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
        GLsizei l;
        std::vector<GLchar> log = {};
        gl->glGetProgramInfoLog(programbase, 0, &l, log.data());
        log.resize(l);
        gl->glGetProgramInfoLog(programbase, 1024, nullptr, log.data());
        throw std::runtime_error(fmt::format("link error: {}", log.data()));
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
    this->blendState = state;
}

void OMRendererPipelineOpenGL::bindInput(int idx, common::OMRendererBuffer *buff)
{
    inputs[idx] = buff;
}
void OMRendererPipelineOpenGL::bindInput(int idx, common::OMRendererTexture *texture)
{
    inputs[idx] = texture;
}
} // namespace openminecraft::renderer::opengl
