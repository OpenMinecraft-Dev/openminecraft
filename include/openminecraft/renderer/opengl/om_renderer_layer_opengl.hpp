#ifndef OM_RENDERER_LAYER_OPENGL_HPP
#define OM_RENDERER_LAYER_OPENGL_HPP

#include "GL/glcorearb.h"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
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
    PFNGLLINKPROGRAMPROC glLinkProgram;
    PFNGLGETPROGRAMIVPROC glGetProgramiv;
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
};
class OMRendererOpenGL : public OMRenderer
{
  public:
    OMRendererOpenGL(AppInfo info, void *window);
    ~OMRendererOpenGL();

    std::string driver() override;
    common::OMRendererBuffer *allocateBuffer(common::OMBufferUsage usage, uint64_t length) override;
    common::OMRendererTexture *allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type,
                                               common::OMTextureArrangement arr) override;
    common::OMRendererRenderTarget *createRenderTarget() override;
    common::OMRendererRenderTarget *getDefaultRenderTarget() override;
    common::OMRendererPipeline *createPipeline() override;
    common::OMRendererTask *createTask() override;
    void registerTask(std::string id, common::OMRendererTask *task) override;
    common::OMRendererTask *fetchTask(std::string id) override;
    void clearTasks() override;
    glm::vec2 getExtent() const override;

    void registerHandler(std::shared_ptr<common::OMRendererHandler>) override;
    void clearHandlers() override;

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
