#ifndef OM_RENDERER_LAYER_OPENGL_HPP
#define OM_RENDERER_LAYER_OPENGL_HPP

#include "GL/glcorearb.h"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/util/om_util_ticker.hpp"
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
    PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer;
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
    PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
    PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
    PFNGLMULTIDRAWELEMENTSINDIRECTPROC glMultiDrawElementsIndirect;
    PFNGLBUFFERSUBDATAPROC glBufferSubData;
    PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
    PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl;
    PFNGLBLENDFUNCPROC glBlendFunc;
    PFNGLDEPTHFUNCPROC glDepthFunc;
    PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
    PFNGLDEPTHMASKPROC glDepthMask;
    PFNGLDRAWELEMENTSINSTANCEDBASEINSTANCEPROC glDrawElementsInstancedBaseInstance;
    PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;
    PFNGLCLEARDEPTHPROC glClearDepth;
    PFNGLCLEARBUFFERFVPROC glClearBufferfv;
    PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
    PFNGLUNIFORM1IPROC glUniform1i;
};
class OMRendererOpenGL : public OMRenderer
{
  public:
    OMRendererOpenGL(AppInfo info, void *window, std::string shaderPath);
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
    auto getLogicalExtent() const -> glm::vec2 override;

    void baseInit() override;

    void render(util::OMTicker &) override;
    void requestResize() override;

    OMRendererOpenGLFuncs gl;
    common::OMRendererRenderTarget *defaultTarget;

  private:
    void *glContext;
    log::OMLogger logger;
    bool needResize = true;

    void initGlFuncs();
};
} // namespace openminecraft::renderer::opengl

#endif
