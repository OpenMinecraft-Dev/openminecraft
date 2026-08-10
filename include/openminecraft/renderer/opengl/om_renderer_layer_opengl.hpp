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
#define OM_OGL_DEF
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_funcs.hpp"
#undef OM_OGL_DEF
};
class OMRendererOpenGL : public OMRenderer
{
  public:
    OMRendererOpenGL(AppInfo info, void *window, std::string shaderPath);
    ~OMRendererOpenGL() override;

    auto driver() -> std::string override;
    auto allocateBuffer(common::OMBufferUsage usage, uint64_t length) -> common::OMRendererBuffer * override;
    auto allocateTexture(uint64_t width, uint64_t height, uint64_t layers, uint64_t mipmap, common::OMTextureType type,
                         common::OMTextureArrangement arr) -> common::OMRendererTexture * override;
    auto createRenderTarget() -> common::OMRendererRenderTarget * override;
    auto getDefaultRenderTarget() -> common::OMRendererRenderTarget * override;
    auto createPipeline() -> common::OMRendererPipeline * override;
    auto createTask(std::string name) -> common::OMRendererTask * override;
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
