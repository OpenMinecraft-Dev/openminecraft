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
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, info.minApiVersion.majorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, info.minApiVersion.minorVersion);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    glContext = SDL_GL_CreateContext(reinterpret_cast<SDL_Window *>(window));
    SDL_GL_MakeCurrent(reinterpret_cast<SDL_Window *>(window), reinterpret_cast<SDL_GLContext>(glContext));

    this->initGlFuncs();

    auto glString = [&](GLenum name) -> std::string {
        const auto *value = gl.glGetString(name);
        return value != nullptr ? std::string(reinterpret_cast<const char *>(value)) : std::string("<unknown>");
    };

    auto *sdlWindow = reinterpret_cast<SDL_Window *>(window);
    const auto pixelFormat = SDL_GetWindowPixelFormat(sdlWindow);

    int red = 0, green = 0, blue = 0, alpha = 0;
    int depth = 0, stencil = 0, doubleBuffer = 0;
    int multisampleBuffers = 0, multisampleSamples = 0;
    int framebufferSrgb = 0;
    SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &red);
    SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &green);
    SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &blue);
    SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &alpha);
    SDL_GL_GetAttribute(SDL_GL_DEPTH_SIZE, &depth);
    SDL_GL_GetAttribute(SDL_GL_STENCIL_SIZE, &stencil);
    SDL_GL_GetAttribute(SDL_GL_DOUBLEBUFFER, &doubleBuffer);
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &multisampleBuffers);
    SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &multisampleSamples);
    SDL_GL_GetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, &framebufferSrgb);

    GLint glSamples = 0;
    GLint glSampleBuffers = 0;
    gl.glGetIntegerv(GL_SAMPLES, &glSamples);
    gl.glGetIntegerv(GL_SAMPLE_BUFFERS, &glSampleBuffers);

    int swapInterval = 0;
    SDL_GL_GetSwapInterval(&swapInterval);

    logger.info("OpenGL vendor: {}", glString(GL_VENDOR));
    logger.info("OpenGL renderer: {}", glString(GL_RENDERER));
    logger.info("OpenGL version: {}", glString(GL_VERSION));
    logger.info("OpenGL shading language: {}", glString(GL_SHADING_LANGUAGE_VERSION));
    logger.info("Window pixel format: {} ({})", SDL_GetPixelFormatName(pixelFormat),
                static_cast<int>(pixelFormat));
    logger.info("Surface format: RGBA={}{}{}{}, depth={}, stencil={}", red, green, blue, alpha, depth, stencil);
    logger.info("Present mode equivalents: doubleBuffer={}, swapInterval={}, MSAA buffers={} samples={} (GL={}/{}), sRGB={}",
                doubleBuffer, swapInterval, multisampleBuffers, multisampleSamples, glSampleBuffers, glSamples,
                framebufferSrgb);

    GLint extensionCount = 0;
    gl.glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    logger.info("OpenGL extensions: {}", extensionCount);

    static constexpr std::array<const char *, 7> swapExtensions = {
        "WGL_EXT_swap_control", "WGL_EXT_swap_control_tear", "GLX_EXT_swap_control",
        "GLX_MESA_swap_control", "GLX_SGI_swap_control", "EGL_KHR_swap_buffers_with_damage",
        "EGL_EXT_swap_buffers_with_damage"};
    for (GLuint i = 0; i < static_cast<GLuint>(extensionCount); ++i)
    {
        const auto *ext = gl.glGetStringi(GL_EXTENSIONS, i);
        if (ext == nullptr)
        {
            continue;
        }

        const std::string name(reinterpret_cast<const char *>(ext));
        for (const auto *wanted : swapExtensions)
        {
            if (name == wanted)
            {
                logger.info("Swap control extension: {}", name);
            }
        }
    }

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
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_funcs.hpp"
}

OMRendererOpenGL::~OMRendererOpenGL()
{
    for (auto tsk : tasks)
    {
        delete tsk.second;
    }
    tasks.clear();
    handlers.clear();

    // Destroy all registered buffers / textures / render targets (must happen before the context is destroyed)
    clearRegistry();

    delete defaultTarget;
    SDL_GL_DestroyContext(reinterpret_cast<SDL_GLContext>(glContext));
}

auto OMRendererOpenGL::driver() -> std::string
{
    auto r = gl.glGetString(GL_RENDERER);
    if (r)
    {
        return reinterpret_cast<const char *>(r);
    }
    return "<unknown>";
}
auto OMRendererOpenGL::allocateBuffer(common::OMBufferUsage usage, uint64_t length) -> common::OMRendererBuffer *
{
    return new OMRendererBufferOpenGL(usage, length, this);
}
auto OMRendererOpenGL::allocateTexture(uint64_t width, uint64_t height, uint64_t layers, uint64_t mipmap,
                                       common::OMTextureType type, common::OMTextureArrangement arr)
    -> common::OMRendererTexture *
{
    return new OMRendererTextureOpenGL(width, height, layers, mipmap, type, arr, this);
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
auto OMRendererOpenGL::createTask(std::string name) -> common::OMRendererTask *
{
    auto task = new OMRendererTaskOpenGL(this);
    registerTask(name, task);
    return task;
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
    {
        auto now = std::chrono::steady_clock::now();

        if (frameStamp != std::chrono::steady_clock::time_point{})
        {
            frameTimeMs = std::chrono::duration<float, std::milli>(now - frameStamp).count();
        }

        frameStamp = now;
    }

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
    runSchedules();
    for (auto h : handlers)
    {
        h->beforeFrame();
    }
    t.pop();

    int i = 0;
    for (auto &tsklist : layeredTasks)
    {
        t.push(fmt::format("gl_task_layer{}", i));
        for (auto tsk : tsklist)
        {
            if (!tsk->isDisabled())
            {
                reinterpret_cast<OMRendererTaskOpenGL *>(tsk)->execute();
            }
        }
        t.pop();
        ++i;
    }

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
