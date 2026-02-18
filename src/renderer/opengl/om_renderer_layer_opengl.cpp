#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "GL/glcorearb.h"
#include "SDL3/SDL_video.h"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"

namespace openminecraft::renderer::opengl
{
OMRendererOpenGL::OMRendererOpenGL(AppInfo info, void *window)
    : OMRenderer(info, window), logger("OMRendererOpenGL", this)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, info.minApiVersion.majorver);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, info.minApiVersion.minorver);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    glContext = SDL_GL_CreateContext(reinterpret_cast<SDL_Window *>(window));

    this->initGlFuncs();
    logger.info("vendor: {}", reinterpret_cast<const char *>(gl.glGetString(GL_RENDERER)));
}

template <typename T> inline T fetchGlFunc(const char *name)
{
    return (T)(SDL_GL_GetProcAddress(name));
}

void OMRendererOpenGL::initGlFuncs()
{
    gl.glGetString = fetchGlFunc<PFNGLGETSTRINGPROC>("glGetString");
    gl.glGetIntegerv = fetchGlFunc<PFNGLGETINTEGERVPROC>("glGetIntegerv");
    gl.glGetError = fetchGlFunc<PFNGLGETERRORPROC>("glGetError");
    gl.glGenBuffers = fetchGlFunc<PFNGLGENBUFFERSPROC>("glGenBuffers");
    gl.glCreateShader = fetchGlFunc<PFNGLCREATESHADERPROC>("glCreateShader");
    gl.glShaderSource = fetchGlFunc<PFNGLSHADERSOURCEPROC>("glShaderSource");
    gl.glCompileShader = fetchGlFunc<PFNGLCOMPILESHADERPROC>("glCompileShader");
    gl.glGetShaderiv = fetchGlFunc<PFNGLGETSHADERIVPROC>("glGetShaderiv");
    gl.glGetShaderInfoLog = fetchGlFunc<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");
    gl.glLinkProgram = fetchGlFunc<PFNGLLINKPROGRAMPROC>("glLinkProgram");
    gl.glGetProgramiv = fetchGlFunc<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");
    gl.glBufferData = fetchGlFunc<PFNGLBUFFERDATAPROC>("glBufferData");
    gl.glBindBuffer = fetchGlFunc<PFNGLBINDBUFFERPROC>("glBindBuffer");
    gl.glDeleteBuffers = fetchGlFunc<PFNGLDELETEBUFFERSPROC>("glDeleteBuffers");
}

OMRendererOpenGL::~OMRendererOpenGL()
{
    SDL_GL_DestroyContext(reinterpret_cast<SDL_GLContext>(glContext));
}

std::string OMRendererOpenGL::driver()
{
    return reinterpret_cast<const char *>(gl.glGetString(GL_RENDERER));
}
common::OMRendererBuffer *OMRendererOpenGL::allocateBuffer(common::OMBufferUsage usage, uint64_t length)
{
    return new OMRendererBufferOpenGL(usage, length, this);
}
common::OMRendererTexture *OMRendererOpenGL::allocateTexture(uint64_t width, uint64_t height,
                                                             common::OMTextureType type,
                                                             common::OMTextureArrangement arr)
{
    return nullptr;
}
common::OMRendererRenderTarget *OMRendererOpenGL::createRenderTarget()
{
    return nullptr;
}
common::OMRendererRenderTarget *OMRendererOpenGL::getDefaultRenderTarget()
{
    return nullptr;
}
common::OMRendererPipeline *OMRendererOpenGL::createPipeline()
{
    return nullptr;
}
common::OMRendererTask *OMRendererOpenGL::createTask()
{
    return nullptr;
}
glm::vec2 OMRendererOpenGL::getExtent() const
{
    return {0.0f, 0.0f};
}
void OMRendererOpenGL::registerTask(std::string id, common::OMRendererTask *task)
{
}

common::OMRendererTask *OMRendererOpenGL::fetchTask(std::string id)
{
    return nullptr;
}

void OMRendererOpenGL::clearTasks()
{
}

void OMRendererOpenGL::registerHandler(std::shared_ptr<common::OMRendererHandler>)
{
}
void OMRendererOpenGL::clearHandlers()
{
}

void OMRendererOpenGL::baseInit()
{
}

void OMRendererOpenGL::render()
{
}

void OMRendererOpenGL::requestResize()
{
}
} // namespace openminecraft::renderer::opengl
