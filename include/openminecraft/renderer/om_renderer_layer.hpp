#ifndef OM_RENDERER_LAYER_HPP
#define OM_RENDERER_LAYER_HPP

#include "common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include <exception>
#include <glm/glm.hpp>
#include <string>

namespace openminecraft::renderer::common
{
class OMRendererHandler;
}

namespace openminecraft::renderer
{
struct AppInfo
{
    std::string appName;
    util::Version appVer;
    std::string engineName;
    util::Version engineVer;
    util::Version minApiVersion;
};
class OMRenderer
{
  public:
    OMRenderer(AppInfo info, void *window);
    virtual ~OMRenderer() = default;

    virtual std::string driver() = 0;
    virtual common::OMRendererBuffer *allocateBuffer(common::OMBufferUsage usage, uint64_t length) = 0;
    virtual common::OMRendererTexture *allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type,
                                                       common::OMTextureArrangement arrangement) = 0;
    virtual common::OMRendererRenderTarget *createRenderTarget() = 0;
    virtual common::OMRendererRenderTarget *getDefaultRenderTarget() = 0;
    virtual common::OMRendererPipeline *createPipeline() = 0;
    virtual common::OMRendererTask *createTask() = 0;
    virtual void registerTask(std::string id, common::OMRendererTask *task) = 0;
    virtual common::OMRendererTask *fetchTask(std::string id) = 0;
    virtual void clearTasks() = 0;

    virtual void registerHandler(std::shared_ptr<common::OMRendererHandler> handler) = 0;
    virtual void clearHandlers() = 0;

    virtual void baseInit() = 0;

    virtual glm::vec2 getExtent() const = 0;

  protected:
    void *window;

  private:
    const AppInfo info;
};
} // namespace openminecraft::renderer

#endif
