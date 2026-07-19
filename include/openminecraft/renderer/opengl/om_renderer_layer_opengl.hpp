#ifndef OM_RENDERER_LAYER_OPENGL_HPP
#define OM_RENDERER_LAYER_OPENGL_HPP

#include "GL/glcorearb.h"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
namespace openminecraft::renderer::opengl
{
struct OMRendererOpenGLFuncs
{
    PFNGLGETERRORPROC glGetError;
    PFNGLGETSTRINGPROC glGetString;
    PFNGLGETINTEGERVPROC glGetIntegerv;
    PFNGLGENBUFFERSPROC glGenBuffers;
    PFNGLCREATESHADERPROC glCreateShader;
    PFNGLSHADERSOURCEPROC glShaderSource;
    PFNGLCOMPILESHADERPROC glCompileShader;
    PFNGLGETSHADERIVPROC glGetShaderiv;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
    PFNGLATTACHSHADERPROC glAttachShader;
    PFNGLCREATEPROGRAMPROC glCreateProgram;
    PFNGLLINKPROGRAMPROC glLinkProgram;
    PFNGLGETPROGRAMIVPROC glGetProgramiv;
    PFNGLDELETEPROGRAMPROC glDeleteProgram;
    PFNGLMAPBUFFERPROC glMapBuffer;
    PFNGLUNMAPBUFFERPROC glUnmapBuffer;
    PFNGLBUFFERDATAPROC glBufferData;
    PFNGLBINDBUFFERPROC glBindBuffer;
    PFNGLDELETEBUFFERSPROC glDeleteBuffers;
    PFNGLGENTEXTURESPROC glGenTextures;
    PFNGLBINDTEXTUREPROC glBindTexture;
    PFNGLTEXIMAGE1DPROC glTexImage1D;
    PFNGLTEXIMAGE2DPROC glTexImage2D;
    PFNGLTEXIMAGE3DPROC glTexImage3D;
    PFNGLDELETETEXTURESPROC glDeleteTextures;
    PFNGLGENRENDERBUFFERSPROC glGenRenderBuffers;
    PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
    PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
    PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
    PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
    PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
    PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
    PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
    PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
    PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
    PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
    PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
    PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
    PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
    PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
    PFNGLDELETESHADERPROC glDeleteShader;
    PFNGLVIEWPORTPROC glViewport;
    PFNGLUSEPROGRAMPROC glUseProgram;
    PFNGLACTIVETEXTUREPROC glActiveTexture;
    PFNGLDRAWARRAYSPROC glDrawArrays;
    PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
    PFNGLBINDBUFFERBASEPROC glBindBufferBase;
    PFNGLDRAWELEMENTSPROC glDrawElements;
    PFNGLCLEARPROC glClear;
    PFNGLENABLEPROC glEnable;
    PFNGLDISABLEPROC glDisable;
    PFNGLTEXPARAMETERIPROC glTexParameteri;
    PFNGLGETPROGRAMBINARYPROC glGetProgramBinary;
    PFNGLPROGRAMBINARYPROC glProgramBinary;
};
class OMRendererOpenGL : public OMRenderer
{
  public:
    OMRendererOpenGL(AppInfo info, void *window);
    ~OMRendererOpenGL() override;

    auto driver() -> std::string override;
    auto allocateBuffer(common::OMBufferUsage usage, uint64_t length) -> common::OMRendererBuffer * override;
    auto allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type, common::OMTextureArrangement arr)
        -> common::OMRendererTexture * override;
    auto createRenderTarget() -> common::OMRendererRenderTarget * override;
    auto getDefaultRenderTarget() -> common::OMRendererRenderTarget * override;
    auto createPipeline() -> common::OMRendererPipeline * override;
    auto createTask() -> common::OMRendererTask * override;
    auto getExtent() const -> glm::vec2 override;

    void baseInit() override;

    void render() override;
    void requestResize() override;

    OMRendererOpenGLFuncs gl;
    common::OMRendererRenderTarget *defaultTarget;

  private:
    void *glContext;
    log::OMLogger logger;

    void initGlFuncs();
};
} // namespace openminecraft::renderer::opengl

#endif
