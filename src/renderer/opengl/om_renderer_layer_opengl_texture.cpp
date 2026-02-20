#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include <iostream>

namespace openminecraft::renderer::opengl
{
static GLenum fromCommon(common::OMTextureType t)
{
    switch (t)
    {
    case common::Dim1:
        return GL_TEXTURE_1D;
    case common::Dim2:
    default:
        return GL_TEXTURE_2D;
    case common::Dim3:
        return GL_TEXTURE_3D;
    }
}

static GLenum fromCommon(common::OMTextureArrangement arr)
{
    switch (arr)
    {
    case common::ColorRgb:
        return GL_RGB;
    case common::ColorRgba:
    default:
        return GL_RGBA;
    case common::Depth:
        return GL_DEPTH24_STENCIL8;
    }
}

OMRendererTextureOpenGL::OMRendererTextureOpenGL(uint64_t width, uint64_t height, common::OMTextureType type,
                                                 common::OMTextureArrangement arr, OMRendererOpenGL *renderer)
    : common::OMRendererTexture(width, height, type, arr, renderer)
{
    this->gl = &renderer->gl;
    if (arr == common::Depth)
    {
        gl->glGenRenderBuffers(1, &texture);
        gl->glBindRenderbuffer(GL_RENDERBUFFER, texture);
        gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    }
    else
    {
        gl->glGenTextures(1, &texture);
        gl->glBindTexture(fromCommon(type), texture);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        gl->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        updateData(nullptr);
    }
}
OMRendererTextureOpenGL::~OMRendererTextureOpenGL()
{
    if (arr == common::Depth)
    {
        gl->glDeleteRenderbuffers(1, &texture);
    }
    else
    {
        gl->glDeleteTextures(1, &texture);
    }
}

void OMRendererTextureOpenGL::updateData(void *d)
{
    gl->glBindTexture(fromCommon(type), texture);
    gl->glTexImage2D(fromCommon(type), 0, fromCommon(arr), width, height, 0, fromCommon(arr), GL_UNSIGNED_BYTE, d);
    gl->glBindTexture(fromCommon(type), 0);
}
} // namespace openminecraft::renderer::opengl
