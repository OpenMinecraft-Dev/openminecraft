#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_rendertarget.hpp"
#include "GL/glcorearb.h"
#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"
#include <algorithm>

namespace openminecraft::renderer::opengl
{
OMRendererRenderTargetOpenGL::OMRendererRenderTargetOpenGL(OMRendererOpenGL *renderer)
    : common::OMRendererRenderTarget(renderer), glrenderer(renderer)
{
    this->gl = &renderer->gl;
}
OMRendererRenderTargetOpenGL::~OMRendererRenderTargetOpenGL()
{
    if (framebuffer != 0)
    {
        gl->glDeleteFramebuffers(1, &framebuffer);
    }
}

void OMRendererRenderTargetOpenGL::attachTarget(common::OMRendererTexture *texture)
{
    textures.push_back(texture);
}
void OMRendererRenderTargetOpenGL::replaceTarget(int idx, common::OMRendererTexture *texture)
{
    textures[idx] = texture;
}
void OMRendererRenderTargetOpenGL::rebuild()
{
    if (framebuffer != 0)
    {
        gl->glDeleteFramebuffers(1, &framebuffer);
        build();
    }
}
glm::vec2 OMRendererRenderTargetOpenGL::fetchSize()
{
    if (framebuffer == 0)
    {
        return glrenderer->getExtent();
    }
    glm::vec2 result = {};
    for (auto p : textures)
    {
        result.x = std::max(static_cast<float>(p->width), result.x);
        result.y = std::max(static_cast<float>(p->height), result.y);
    }

    return result;
}
void OMRendererRenderTargetOpenGL::build()
{
    if (textures.empty())
    {
        framebuffer = 0;
    }
    else
    {
        gl->glGenFramebuffers(1, &framebuffer);
        gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        int i = 0;
        for (auto tt : textures)
        {
            if (tt->arr == common::Depth)
            {
                gl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                              reinterpret_cast<OMRendererTextureOpenGL *>(tt)->texture);
            }
            else
            {
                gl->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D,
                                           reinterpret_cast<OMRendererTextureOpenGL *>(tt)->texture, 0);
                i++;
            }
        }
    }

    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
} // namespace openminecraft::renderer::opengl
