#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "GL/glcorearb.h"
#include "SDL3/SDL_video.h"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_rendertarget.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_task.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"

namespace openminecraft::renderer::opengl
{
OMRendererOpenGL::OMRendererOpenGL(AppInfo info, void *window)
    : OMRenderer(info, window), logger("OMRendererOpenGL", this)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, info.minApiVersion.majorver);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, info.minApiVersion.minorver);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    glContext = SDL_GL_CreateContext(reinterpret_cast<SDL_Window *>(window));
    SDL_GL_MakeCurrent(reinterpret_cast<SDL_Window *>(window), reinterpret_cast<SDL_GLContext>(glContext));

    this->initGlFuncs();

    defaultTarget = createRenderTarget();
    defaultTarget->build();
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
    gl.glAttachShader = fetchGlFunc<PFNGLATTACHSHADERPROC>("glAttachShader");
    gl.glCreateProgram = fetchGlFunc<PFNGLCREATEPROGRAMPROC>("glCreateProgram");
    gl.glLinkProgram = fetchGlFunc<PFNGLLINKPROGRAMPROC>("glLinkProgram");
    gl.glGetProgramiv = fetchGlFunc<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");
    gl.glDeleteProgram = fetchGlFunc<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");
    gl.glBufferData = fetchGlFunc<PFNGLBUFFERDATAPROC>("glBufferData");
    gl.glBindBuffer = fetchGlFunc<PFNGLBINDBUFFERPROC>("glBindBuffer");
    gl.glDeleteBuffers = fetchGlFunc<PFNGLDELETEBUFFERSPROC>("glDeleteBuffers");
    gl.glGenTextures = fetchGlFunc<PFNGLGENTEXTURESPROC>("glGenTextures");
    gl.glBindTexture = fetchGlFunc<PFNGLBINDTEXTUREPROC>("glBindTexture");
    gl.glTexImage1D = fetchGlFunc<PFNGLTEXIMAGE1DPROC>("glTexImage1D");
    gl.glTexImage2D = fetchGlFunc<PFNGLTEXIMAGE2DPROC>("glTexImage2D");
    gl.glTexImage3D = fetchGlFunc<PFNGLTEXIMAGE3DPROC>("glTexImage3D");
    gl.glDeleteTextures = fetchGlFunc<PFNGLDELETETEXTURESPROC>("glDeleteTextures");
    gl.glGenRenderBuffers = fetchGlFunc<PFNGLGENRENDERBUFFERSPROC>("glGenRenderbuffers");
    gl.glBindRenderbuffer = fetchGlFunc<PFNGLBINDRENDERBUFFERPROC>("glBindRenderbuffer");
    gl.glRenderbufferStorage = fetchGlFunc<PFNGLRENDERBUFFERSTORAGEPROC>("glRenderbufferStorage");
    gl.glDeleteRenderbuffers = fetchGlFunc<PFNGLDELETERENDERBUFFERSPROC>("glDeleteRenderbuffers");
    gl.glGenFramebuffers = fetchGlFunc<PFNGLGENFRAMEBUFFERSPROC>("glGenFramebuffers");
    gl.glBindFramebuffer = fetchGlFunc<PFNGLBINDFRAMEBUFFERPROC>("glBindFramebuffer");
    gl.glFramebufferTexture2D = fetchGlFunc<PFNGLFRAMEBUFFERTEXTURE2DPROC>("glFramebufferTexture2D");
    gl.glFramebufferRenderbuffer = fetchGlFunc<PFNGLFRAMEBUFFERRENDERBUFFERPROC>("glFramebufferRenderbuffer");
    gl.glCheckFramebufferStatus = fetchGlFunc<PFNGLCHECKFRAMEBUFFERSTATUSPROC>("glCheckFramebufferStatus");
    gl.glDeleteFramebuffers = fetchGlFunc<PFNGLDELETEFRAMEBUFFERSPROC>("glDeleteFramebuffers");
    gl.glGenVertexArrays = fetchGlFunc<PFNGLGENVERTEXARRAYSPROC>("glGenVertexArrays");
    gl.glBindVertexArray = fetchGlFunc<PFNGLBINDVERTEXARRAYPROC>("glBindVertexArray");
    gl.glVertexAttribPointer = fetchGlFunc<PFNGLVERTEXATTRIBPOINTERPROC>("glVertexAttribPointer");
    gl.glEnableVertexAttribArray = fetchGlFunc<PFNGLENABLEVERTEXATTRIBARRAYPROC>("glEnableVertexAttribArray");
    gl.glDeleteVertexArrays = fetchGlFunc<PFNGLDELETEVERTEXARRAYSPROC>("glDeleteVertexArrays");
    gl.glGetProgramInfoLog = fetchGlFunc<PFNGLGETPROGRAMINFOLOGPROC>("glGetProgramInfoLog");
    gl.glDeleteShader = fetchGlFunc<PFNGLDELETESHADERPROC>("glDeleteShader");
    gl.glViewport = fetchGlFunc<PFNGLVIEWPORTPROC>("glViewport");
    gl.glUseProgram = fetchGlFunc<PFNGLUSEPROGRAMPROC>("glUseProgram");
    gl.glActiveTexture = fetchGlFunc<PFNGLACTIVETEXTUREPROC>("glActiveTexture");
    gl.glDrawArrays = fetchGlFunc<PFNGLDRAWARRAYSPROC>("glDrawArrays");
    gl.glUniformBlockBinding = fetchGlFunc<PFNGLUNIFORMBLOCKBINDINGPROC>("glUniformBlockBinding");
    gl.glBindBufferBase = fetchGlFunc<PFNGLBINDBUFFERBASEPROC>("glBindBufferBase");
    gl.glDrawElements = fetchGlFunc<PFNGLDRAWELEMENTSPROC>("glDrawElements");
    gl.glClear = fetchGlFunc<PFNGLCLEARPROC>("glClear");
    gl.glEnable = fetchGlFunc<PFNGLENABLEPROC>("glEnable");
    gl.glDisable = fetchGlFunc<PFNGLDISABLEPROC>("glDisable");
    gl.glTexParameteri = fetchGlFunc<PFNGLTEXPARAMETERIPROC>("glTexParameteri");
    gl.glProgramBinary = fetchGlFunc<PFNGLPROGRAMBINARYPROC>("glProgramBinary");
    gl.glGetProgramBinary = fetchGlFunc<PFNGLGETPROGRAMBINARYPROC>("glGetProgramBinary");
}

OMRendererOpenGL::~OMRendererOpenGL()
{
    for (auto tsk : tasks)
    {
        delete tsk.second;
    }
    tasks.clear();
    handlers.clear();
    delete defaultTarget;
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
    return new OMRendererTextureOpenGL(width, height, type, arr, this);
}
common::OMRendererRenderTarget *OMRendererOpenGL::createRenderTarget()
{
    return new OMRendererRenderTargetOpenGL(this);
}
common::OMRendererRenderTarget *OMRendererOpenGL::getDefaultRenderTarget()
{
    return defaultTarget;
}
common::OMRendererPipeline *OMRendererOpenGL::createPipeline()
{
    return new OMRendererPipelineOpenGL(this);
}
common::OMRendererTask *OMRendererOpenGL::createTask()
{
    return new OMRendererTaskOpenGL(this);
}
glm::vec2 OMRendererOpenGL::getExtent() const
{
    int w, h;
    SDL_GetWindowSize(reinterpret_cast<SDL_Window *>(window), &w, &h);
    return {w, h};
}
void OMRendererOpenGL::registerTask(std::string id, common::OMRendererTask *task)
{
    tasks[id] = task;
}

common::OMRendererTask *OMRendererOpenGL::fetchTask(std::string id)
{
    return tasks[id];
}

void OMRendererOpenGL::clearTasks()
{
    for (auto t : tasks)
    {
        delete t.second;
    }
    tasks.clear();
}

void OMRendererOpenGL::registerHandler(std::shared_ptr<common::OMRendererHandler> h)
{
    handlers.push_back(h);
}
void OMRendererOpenGL::clearHandlers()
{
    handlers.clear();
}

void OMRendererOpenGL::baseInit()
{

    for (auto h : handlers)
    {
        h->submitTasks();
    }
}

void OMRendererOpenGL::render()
{
    for (auto h : handlers)
    {
        h->beforeFrame();
    }
    for (auto tsks : tasks)
    {
        reinterpret_cast<OMRendererTaskOpenGL *>(tsks.second)->execute();
    }
    SDL_GL_SwapWindow(reinterpret_cast<SDL_Window *>(window));
    for (auto h : handlers)
    {
        h->afterFrame();
    }
}

void OMRendererOpenGL::requestResize()
{
    auto siz = getExtent();
    gl.glViewport(0, 0, siz.x, siz.y);

    for (auto tsk : tasks)
    {
        delete tsk.second;
    }
    tasks.clear();

    for (auto h : handlers)
    {
        h->submitTasks();
    }
}
} // namespace openminecraft::renderer::opengl
