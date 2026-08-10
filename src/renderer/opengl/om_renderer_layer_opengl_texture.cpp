#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"
#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include <array>
#include <cstdint>

namespace openminecraft::renderer::opengl
{
static auto fromCommon(common::OMTextureType t) -> GLenum
{
    switch (t)
    {
    case common::Dim2:
    default:
        return GL_TEXTURE_2D;
    case common::Dim2Array:
        return GL_TEXTURE_2D_ARRAY;
    }
}

static auto fromCommon(common::OMTextureArrangement arr) -> GLenum
{
    switch (arr)
    {
    case common::ColorRgb:
        return GL_RGB;
    case common::ColorRgba:
    default:
        return GL_RGBA;
    case common::Depth:
        return GL_DEPTH_COMPONENT32F;
    }
}

static auto fromCommonI(common::OMTextureArrangement arr) -> GLenum
{
    switch (arr)
    {
    case common::ColorRgb:
        return GL_SRGB8;
    case common::ColorRgba:
    default:
        return GL_SRGB8_ALPHA8;
    case common::Depth:
        return GL_DEPTH_COMPONENT32F;
    }
}

static auto fromCommon(common::OMTextureAddressMode a) -> GLenum
{
    switch (a)
    {
    case common::Repeat:
        return GL_REPEAT;
    default:
    case common::ClampToEdge:
        return GL_CLAMP_TO_EDGE;
    case common::ClampToBorder:
        return GL_CLAMP_TO_BORDER;
    }
}

static std::array<float, 4> transparentBlack = {0.0f, 0.0f, 0.0f, 0.0f};
static std::array<float, 4> opaqueBlack = {0.0f, 0.0f, 0.0f, 1.0f};
static std::array<float, 4> opaqueWhite = {1.0f, 1.0f, 1.0f, 1.0f};

static auto fromCommon(common::OMTextureBorder b) -> float *
{
    switch (b)
    {
    case common::TransparentBlack:
        return transparentBlack.data();
    case common::OpaqueWhite:
        return opaqueWhite.data();
    default:
    case common::OpaqueBlack:
        return opaqueBlack.data();
    }
}

OMRendererTextureOpenGL::OMRendererTextureOpenGL(uint64_t width, uint64_t height, uint64_t layers, uint64_t mipmap,
                                                 common::OMTextureType type, common::OMTextureArrangement arr,
                                                 OMRendererOpenGL *renderer)
    : common::OMRendererTexture(width, height, layers, mipmap, type, arr, renderer), mipmap(mipmap), layers(layers)
{
    this->gl = &renderer->gl;
    if (arr == common::Depth)
    {
        gl->glGenRenderbuffers(1, &texture);
        gl->glBindRenderbuffer(GL_RENDERBUFFER, texture);
        gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    }
    else
    {
        gl->glGenTextures(1, &texture);
        allocateBase();
    }
}

void OMRendererTextureOpenGL::allocateBase()
{
    gl->glBindTexture(fromCommon(type), texture);
    switch (type)
    {
    case common::Dim2:
        gl->glTexImage2D(fromCommon(type), 0, fromCommonI(arr), width, height, 0, fromCommon(arr), GL_UNSIGNED_BYTE,
                         nullptr);
        break;
    case common::Dim2Array:
        gl->glTexImage3D(fromCommon(type), 0, fromCommonI(arr), width, height, layers, 0, fromCommon(arr),
                         GL_UNSIGNED_BYTE, nullptr);
        break;
    }
    gl->glBindTexture(fromCommon(type), 0);
}

static auto toFilter(common::OMTextureFilter f, common::OMTextureFilter mip) -> GLenum
{
    switch (f << 1 | mip)
    {
    default:
    case 0b00:
        return GL_LINEAR_MIPMAP_LINEAR;
    case 0b01:
        return GL_LINEAR_MIPMAP_NEAREST;
    case 0b10:
        return GL_NEAREST_MIPMAP_LINEAR;
    case 0b11:
        return GL_NEAREST_MIPMAP_NEAREST;
    }
}

static auto toFilter2(common::OMTextureFilter f) -> GLenum
{
    switch (f)
    {
    default:
    case common::Linear:
        return GL_LINEAR;
    case common::Nearest:
        return GL_NEAREST;
    }
}

void OMRendererTextureOpenGL::setupSampler()
{
    gl->glBindTexture(fromCommon(type), texture);
    gl->glTexParameteri(fromCommon(type), GL_TEXTURE_MAX_LEVEL, mipmap);
    if (mipmap > 0)
    {
        gl->glTexParameteri(fromCommon(type), GL_TEXTURE_MIN_FILTER, toFilter(minFilter, mipFilter));
        gl->glTexParameteri(fromCommon(type), GL_TEXTURE_MAG_FILTER, toFilter(magFilter, mipFilter));
    }
    else
    {
        gl->glTexParameteri(fromCommon(type), GL_TEXTURE_MIN_FILTER, toFilter2(minFilter));
        gl->glTexParameteri(fromCommon(type), GL_TEXTURE_MAG_FILTER, toFilter2(magFilter));
    }
    gl->glTexParameteri(fromCommon(type), GL_TEXTURE_WRAP_S, fromCommon(addressModeU));
    gl->glTexParameteri(fromCommon(type), GL_TEXTURE_WRAP_T, fromCommon(addressModeV));
    gl->glTexParameteri(fromCommon(type), GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    gl->glTexParameterfv(fromCommon(type), GL_TEXTURE_BORDER_COLOR, fromCommon(border));
    if (mipmap > 0)
    {
        gl->glGenerateMipmap(fromCommon(type));
    }
    gl->glBindTexture(fromCommon(type), 0);
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

void OMRendererTextureOpenGL::updateData(void *d, uint64_t layer)
{
    gl->glBindTexture(fromCommon(type), texture);
    switch (type)
    {
    case common::OMTextureType::Dim2:
        gl->glTexImage2D(fromCommon(type), 0, fromCommonI(arr), width, height, 0, fromCommon(arr), GL_UNSIGNED_BYTE, d);
        break;
    case common::OMTextureType::Dim2Array:
        gl->glTexSubImage3D(fromCommon(type), 0, 0, 0, layer, width, height, 1, fromCommon(arr), GL_UNSIGNED_BYTE, d);
        break;
    }
    gl->glBindTexture(fromCommon(type), 0);
}

void OMRendererTextureOpenGL::updateDataPart(void *p, uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t layer)
{
    gl->glBindTexture(fromCommon(type), texture);
    switch (type)
    {
    case common::OMTextureType::Dim2:
        gl->glTexSubImage2D(fromCommon(type), 0, x, y, w, h, fromCommon(arr), GL_UNSIGNED_BYTE, p);
        break;
    case common::OMTextureType::Dim2Array:
        gl->glTexSubImage3D(fromCommon(type), 0, x, y, layer, w, h, 1, fromCommon(arr), GL_UNSIGNED_BYTE, p);
        break;
    }
    gl->glBindTexture(fromCommon(type), 0);
}
} // namespace openminecraft::renderer::opengl
