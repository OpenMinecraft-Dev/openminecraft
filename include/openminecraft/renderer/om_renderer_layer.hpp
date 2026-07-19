#ifndef OM_RENDERER_LAYER_HPP
#define OM_RENDERER_LAYER_HPP

#include "common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_shadercompiler.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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

    virtual auto driver() -> std::string = 0;
    virtual auto allocateBuffer(common::OMBufferUsage usage, uint64_t length) -> common::OMRendererBuffer * = 0;
    virtual auto allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type,
                                 common::OMTextureArrangement arrangement) -> common::OMRendererTexture * = 0;
    virtual auto createRenderTarget() -> common::OMRendererRenderTarget * = 0;
    virtual auto getDefaultRenderTarget() -> common::OMRendererRenderTarget * = 0;
    virtual auto createPipeline() -> common::OMRendererPipeline * = 0;
    virtual auto createTask() -> common::OMRendererTask * = 0;
    inline void registerTask(std::string id, common::OMRendererTask *task)
    {
        tasks[id] = task;
    }
    inline auto fetchTask(std::string id) -> common::OMRendererTask *
    {
        return tasks[id];
    }
    inline void clearTasks()
    {
        for (auto &p : tasks)
        {
            delete p.second;
        }
        tasks.clear();
    }

    inline void registerHandler(std::shared_ptr<common::OMRendererHandler> handler)
    {
        handlers.push_back(handler);
    }
    inline void clearHandlers()
    {
        handlers.clear();
    }

    virtual void baseInit() = 0;

    virtual auto getExtent() const -> glm::vec2 = 0;

    virtual void render() = 0;
    virtual void requestResize() = 0;

    common::OMRendererShaderCompiler compiler;

  protected:
    void *window;
    std::unordered_map<std::string, common::OMRendererTask *> tasks;
    std::vector<std::shared_ptr<common::OMRendererHandler>> handlers;

  private:
    const AppInfo info;
};
} // namespace openminecraft::renderer

#endif
