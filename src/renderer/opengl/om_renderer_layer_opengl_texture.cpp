#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"

namespace openminecraft::renderer::opengl
{
static GLenum fromCommon(common::OMTextureType t)
{
    switch (t)
    {
    case common::Dim1:
        return GL_TEXTURE_1D;
    case common::Dim2:
        return GL_TEXTURE_2D;
    case common::Dim3:
        return GL_TEXTURE_3D;
    default:
        break;
    }
}

static GLenum fromCommon(common::OMTextureArrangement arr)
{
    switch (arr)
    {
    case common::ColorRgb:
        return GL_RGB;
    case common::ColorRgba:
        return GL_RGBA;
    case common::Depth:
        return GL_DEPTH;
    }
}

OMRendererTextureOpenGL::OMRendererTextureOpenGL(uint64_t width, uint64_t height, common::OMTextureType type,
                                                 common::OMTextureArrangement arr, OMRendererOpenGL *renderer)
    : common::OMRendererTexture(width, height, type, arr, renderer)
{
    this->gl = &renderer->gl;
    gl->glGenTextures(1, &texture);
}
OMRendererTextureOpenGL::~OMRendererTextureOpenGL()
{
    gl->glDeleteTextures(1, &texture);
}

void OMRendererTextureOpenGL::updateData(void *d)
{
    gl->glBindTexture(fromCommon(type), texture);
    gl->glTexImage2D(fromCommon(type), 0, fromCommon(arr), width, height, 0, fromCommon(arr), GL_UNSIGNED_BYTE, d);
    gl->glBindTexture(fromCommon(type), 0);
}
} // namespace openminecraft::renderer::opengl
