#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "GL/glcorearb.h"
#include "SDL3/SDL_video.h"
#include "fmt/format.h"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_buffer.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_sync_point.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_rendertarget.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_task.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_texture.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/util/om_util_ticker.hpp"
#include <array>
#include <fstream>
#include <cstdlib>
#include <vector>

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
    frameContext = std::make_unique<OMRendererFrameContextOpenGL>(this, 0);

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

auto OMRendererOpenGL::deviceInfos() -> std::string
{
    std::string out;
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

    out += fmt::format("OpenGL vendor: {}\n", glString(GL_VENDOR));
    out += fmt::format("OpenGL renderer: {}\n", glString(GL_RENDERER));
    out += fmt::format("OpenGL version: {}\n", glString(GL_VERSION));
    out += fmt::format("OpenGL shading language: {}\n", glString(GL_SHADING_LANGUAGE_VERSION));
    out += fmt::format("Window pixel format: {} ({})\n", SDL_GetPixelFormatName(pixelFormat),
                       static_cast<int>(pixelFormat));
    out += fmt::format("Surface format: RGBA={}{}{}{}, depth={}, stencil={}\n", red, green, blue, alpha, depth,
                       stencil);
    out += fmt::format("Present mode equivalents: doubleBuffer={}, swapInterval={}, MSAA buffers={} samples={} "
                       "(GL={}/{}), sRGB={}\n",
                       doubleBuffer, swapInterval, multisampleBuffers, multisampleSamples, glSampleBuffers, glSamples,
                       framebufferSrgb);

    int contextMajor = 0, contextMinor = 0, contextFlags = 0, contextProfile = 0;
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &contextMajor);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &contextMinor);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
    SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &contextProfile);
    out += fmt::format("Context: requested={}.{}, flags=0x{:x}, profile=0x{:x}\n", contextMajor, contextMinor,
                       contextFlags, contextProfile);

    if (const auto *details = SDL_GetPixelFormatDetails(pixelFormat); details != nullptr)
    {
        out += fmt::format("Pixel format details: bpp={}, bytes={}, R={}bits/{}shift/0x{:x}, "
                           "G={}bits/{}shift/0x{:x}, B={}bits/{}shift/0x{:x}, A={}bits/{}shift/0x{:x}\n",
                           details->bits_per_pixel, details->bytes_per_pixel, details->Rbits, details->Rshift,
                           details->Rmask, details->Gbits, details->Gshift, details->Gmask, details->Bbits,
                           details->Bshift, details->Bmask, details->Abits, details->Ashift, details->Amask);
    }

    GLint maxTextureSize = 0, maxCubeSize = 0, max3DSize = 0, maxArrayLayers = 0;
    GLint maxTextureUnits = 0, maxCombinedUnits = 0, maxVertexAttribs = 0;
    GLint maxVertexUniforms = 0, maxFragmentUniforms = 0;
    GLint maxUniformBufferBindings = 0, maxUniformBlockSize = 0;
    GLint maxShaderStorageBindings = 0, maxShaderStorageBlockSize = 0;
    GLint maxDrawBuffers = 0, maxColorAttachments = 0, maxSamples = 0;
    GLint maxViewport[2] = {0, 0};
    GLfloat lineWidth[2] = {0.0f, 0.0f};
    GLfloat pointSize[2] = {0.0f, 0.0f};
    gl.glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    gl.glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxCubeSize);
    gl.glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &max3DSize);
    gl.glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxArrayLayers);
    gl.glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    gl.glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedUnits);
    gl.glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    gl.glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &maxVertexUniforms);
    gl.glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &maxFragmentUniforms);
    gl.glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &maxUniformBufferBindings);
    gl.glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    gl.glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxShaderStorageBindings);
    gl.glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxShaderStorageBlockSize);
    gl.glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    gl.glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    gl.glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    gl.glGetIntegerv(GL_MAX_VIEWPORT_DIMS, maxViewport);
    gl.glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidth);
    gl.glGetFloatv(GL_POINT_SIZE_RANGE, pointSize);

    out += fmt::format("Limits: texture2D={}, cube={}, texture3D={}, arrayLayers={}, "
                       "textureUnits={}, combinedUnits={}, vertexAttribs={}, vertexUniforms={}, "
                       "fragmentUniforms={}, uniformBindings={}, uniformBlockSize={}, "
                       "shaderStorageBindings={}, shaderStorageBlockSize={}, drawBuffers={}, "
                       "colorAttachments={}, maxSamples={}, maxViewport={}x{}\n",
                       maxTextureSize, maxCubeSize, max3DSize, maxArrayLayers, maxTextureUnits, maxCombinedUnits,
                       maxVertexAttribs, maxVertexUniforms, maxFragmentUniforms, maxUniformBufferBindings,
                       maxUniformBlockSize, maxShaderStorageBindings, maxShaderStorageBlockSize, maxDrawBuffers,
                       maxColorAttachments, maxSamples, maxViewport[0], maxViewport[1]);
    out += fmt::format("Ranges: lineWidth={:.2f}..{:.2f}, pointSize={:.2f}..{:.2f}\n",
                       static_cast<double>(lineWidth[0]), static_cast<double>(lineWidth[1]),
                       static_cast<double>(pointSize[0]), static_cast<double>(pointSize[1]));

    GLint extensionCount = 0;
    gl.glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
    out += fmt::format("OpenGL extensions ({}):\n", extensionCount);

    for (GLuint i = 0; i < static_cast<GLuint>(extensionCount); ++i)
    {
        const auto *ext = gl.glGetStringi(GL_EXTENSIONS, i);
        if (ext != nullptr)
        {
            out += fmt::format("  {}\n", reinterpret_cast<const char *>(ext));
        }
    }

    return out;
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

    // Release retired resources while the GL context is still current.
    lifeCycle->clear();

    // Destroy all registered buffers / textures / render targets (must happen before the context is destroyed)
    clearRegistry();

    delete defaultTarget;
    SDL_GL_DestroyContext(reinterpret_cast<SDL_GLContext>(glContext));
}

auto OMRendererOpenGL::createGpuEvent() -> common::OMRendererGpuEvent *
{
    return new OMRendererGpuEventOpenGL();
}

auto OMRendererOpenGL::createGpuCpuBridge(bool) -> common::OMRendererGpuCpuBridge *
{
    return new OMRendererGpuCpuBridgeOpenGL();
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
        gl.glDisable(GL_BLEND);
        gl.glDisable(GL_CULL_FACE);
        gl.glDisable(GL_SCISSOR_TEST);
        gl.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        gl.glDepthMask(GL_TRUE);
        gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
    t.push("gl_lifecycle");
    lifeCycle->collect([this](uint32_t frameIndex, uint64_t frameSerial) {
        return isFrameComplete(frameIndex, frameSerial);
    });
    lifeCycle->setFrameTag(0, 0);
    t.pop();

    t.push("gl_pretasks");
    runSchedules();
    for (auto h : handlers)
    {
        h->beforeFrame();
    }
    t.pop();

    t.push("gl_execute_tasks");
    frameContext->executeTasks(layeredTasks);
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
