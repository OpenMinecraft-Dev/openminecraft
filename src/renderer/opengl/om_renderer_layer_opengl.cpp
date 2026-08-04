#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "GL/glcorearb.h"
#include "SDL3/SDL_video.h"
#include "fmt/format.h"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_rendertarget.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_task.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/util/om_util_ticker.hpp"

namespace openminecraft::renderer::opengl
{
static log::OMLogger logger("OpenGL Debug");
void APIENTRY debugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                            const GLchar *message, const void *userParam)
{
    switch (severity)
    {
    default:
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        logger.debug("{}", message);
        break;
    case GL_DEBUG_SEVERITY_LOW:
        logger.info("{}", message);
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        logger.warn("{}", message);
        break;
    case GL_DEBUG_SEVERITY_HIGH:
        logger.error("{}", message);
        break;
    }
}

OMRendererOpenGL::OMRendererOpenGL(AppInfo info, void *window, std::string shaderPath)
    : OMRenderer(info, window, shaderPath), logger("OMRendererOpenGL", this)
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, info.minApiVersion.majorver);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, info.minApiVersion.minorver);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    glContext = SDL_GL_CreateContext(reinterpret_cast<SDL_Window *>(window));
    SDL_GL_MakeCurrent(reinterpret_cast<SDL_Window *>(window), reinterpret_cast<SDL_GLContext>(glContext));

    this->initGlFuncs();

    defaultTarget = createRenderTarget();
    defaultTarget->build();

    if (false)
    {
        gl.glEnable(GL_DEBUG_OUTPUT);
        gl.glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

        gl.glDebugMessageCallback(debugCallback, nullptr);

        gl.glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
}

template <typename T> inline auto fetchGlFunc(const char *name) -> T
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
    gl.glVertexAttribIPointer = fetchGlFunc<PFNGLVERTEXATTRIBIPOINTERPROC>("glVertexAttribIPointer");
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
    gl.glDrawElementsInstanced = fetchGlFunc<PFNGLDRAWELEMENTSINSTANCEDPROC>("glDrawElementsInstanced");
    gl.glVertexAttribDivisor = fetchGlFunc<PFNGLVERTEXATTRIBDIVISORPROC>("glVertexAttribDivisor");
    gl.glMultiDrawElementsIndirect = fetchGlFunc<PFNGLMULTIDRAWELEMENTSINDIRECTPROC>("glMultiDrawElementsIndirect");
    gl.glBufferSubData = fetchGlFunc<PFNGLBUFFERSUBDATAPROC>("glBufferSubData");
    gl.glDebugMessageCallback = fetchGlFunc<PFNGLDEBUGMESSAGECALLBACKPROC>("glDebugMessageCallback");
    gl.glDebugMessageControl = fetchGlFunc<PFNGLDEBUGMESSAGECONTROLPROC>("glDebugMessageControl");
    gl.glBlendFunc = fetchGlFunc<PFNGLBLENDFUNCPROC>("glBlendFunc");
    gl.glDepthFunc = fetchGlFunc<PFNGLDEPTHFUNCPROC>("glDepthFunc");
    gl.glBlendFuncSeparate = fetchGlFunc<PFNGLBLENDFUNCSEPARATEPROC>("glBlendFuncSeparate");
    gl.glDepthMask = fetchGlFunc<PFNGLDEPTHMASKPROC>("glDepthMask");
    gl.glDrawElementsInstancedBaseInstance =
        fetchGlFunc<PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC>("glDrawElementsInstancedBaseInstance");
    gl.glTexSubImage2D = fetchGlFunc<PFNGLTEXSUBIMAGE2DPROC>("glTexSubImage2D");
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

auto OMRendererOpenGL::driver() -> std::string
{
    return reinterpret_cast<const char *>(gl.glGetString(GL_RENDERER));
}
auto OMRendererOpenGL::allocateBuffer(common::OMBufferUsage usage, uint64_t length) -> common::OMRendererBuffer *
{
    return new OMRendererBufferOpenGL(usage, length, this);
}
auto OMRendererOpenGL::allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type,
                                       common::OMTextureArrangement arr) -> common::OMRendererTexture *
{
    return new OMRendererTextureOpenGL(width, height, type, arr, this);
}
auto OMRendererOpenGL::createRenderTarget() -> common::OMRendererRenderTarget *
{
    return new OMRendererRenderTargetOpenGL(this);
}
auto OMRendererOpenGL::getDefaultRenderTarget() -> common::OMRendererRenderTarget *
{
    return defaultTarget;
}
auto OMRendererOpenGL::createPipeline() -> common::OMRendererPipeline *
{
    return new OMRendererPipelineOpenGL(this);
}
auto OMRendererOpenGL::createTask() -> common::OMRendererTask *
{
    return new OMRendererTaskOpenGL(this);
}
auto OMRendererOpenGL::getExtent() const -> glm::vec2
{
    int w, h;
    SDL_GetWindowSizeInPixels(reinterpret_cast<SDL_Window *>(window), &w, &h);
    return {w, h};
}
auto OMRendererOpenGL::getLogicalExtent() const -> glm::vec2
{
    int w, h;
    SDL_GetWindowSize(reinterpret_cast<SDL_Window *>(window), &w, &h);
    return {w, h};
}

void OMRendererOpenGL::baseInit()
{
    for (auto h : handlers)
    {
        h->submitTasks();
    }
    buildTaskGraph();
}

void OMRendererOpenGL::render(util::OMTicker &t)
{
    t.push("gl_render");
    if (needResize)
    {
        t.push("gl_resize");
        auto siz = getExtent();
        gl.glViewport(0, 0, siz.x, siz.y);

        for (auto tsk : tasks)
        {
            delete tsk.second;
        }
        tasks.clear();

        t.push("gl_tasksubmit");
        for (auto h : handlers)
        {
            h->submitTasks();
        }
        buildTaskGraph();
        t.pop();

        needResize = false;
        t.pop();
    }
    t.push("gl_pretasks");
    for (auto h : handlers)
    {
        h->beforeFrame();
    }
    t.pop();

    t.push("gl_tasks");
    int i = 0;
    for (auto &tsklist : layeredTasks)
    {
        t.push(fmt::format("gl_task_layer{}", i));
        for (auto tsk : tsklist)
        {
            reinterpret_cast<OMRendererTaskOpenGL *>(tsk)->execute();
        }
        t.pop();
        ++i;
    }
    t.pop();

    t.push("gl_swap");
    SDL_GL_SwapWindow(reinterpret_cast<SDL_Window *>(window));
    t.pop();

    t.push("gl_posttasks");
    for (auto h : handlers)
    {
        h->afterFrame();
    }
    t.pop();

    t.pop();
}

void OMRendererOpenGL::requestResize()
{
    needResize = true;
}
} // namespace openminecraft::renderer::opengl
