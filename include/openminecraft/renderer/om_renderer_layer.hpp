#ifndef OM_RENDERER_LAYER_HPP
#define OM_RENDERER_LAYER_HPP

#include "common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_shadercompiler.hpp"
#include "openminecraft/renderer/common/om_renderer_shadermanager.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/util/om_util_ticker.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include <glm/glm.hpp>
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
    OMRenderer(AppInfo info, void *window, std::string shaderPath);
    virtual ~OMRenderer() = default;

    virtual auto driver() -> std::string = 0;
    virtual auto allocateBuffer(common::OMBufferUsage usage, uint64_t length) -> common::OMRendererBuffer * = 0;
    inline auto allocateTexture(uint64_t width, uint64_t height, uint64_t mipmap, common::OMTextureType type,
                                common::OMTextureArrangement arrangement) -> common::OMRendererTexture *
    {
        return allocateTexture(width, height, 1, mipmap, type, arrangement);
    }
    virtual auto allocateTexture(uint64_t width, uint64_t height, uint64_t layers, uint64_t mipmap,
                                 common::OMTextureType type, common::OMTextureArrangement arrangement)
        -> common::OMRendererTexture * = 0;
    virtual auto createRenderTarget() -> common::OMRendererRenderTarget * = 0;
    virtual auto getDefaultRenderTarget() -> common::OMRendererRenderTarget * = 0;
    virtual auto createPipeline() -> common::OMRendererPipeline * = 0;
    virtual auto createTask(std::string name) -> common::OMRendererTask * = 0;

  protected:
    inline void registerTask(std::string id, common::OMRendererTask *task)
    {
        tasks[id] = task;
    }

  public:
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
    virtual auto taskRecreate(std::string id) -> void
    {
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
    virtual auto getLogicalExtent() const -> glm::vec2 = 0;

    virtual void render(util::OMTicker &) = 0;
    virtual void requestResize() = 0;

    common::OMRendererShaderCompiler compiler;

    void buildTaskGraph()
    {
        layeredTasks.clear();

        std::vector<common::OMRendererTask *> temp;
        for (auto &p : tasks)
        {
            temp.push_back(p.second);
            p.second->solved = false;
        }

        while (true)
        {
            std::vector<common::OMRendererTask *> layerTask = {};
            for (auto it = temp.begin(); it < temp.end(); ++it)
            {
                if ((*it)->executable())
                {
                    layerTask.emplace_back(*it);
                    (*it)->solved = true;
                    it = temp.erase(it);

                    if (it == temp.end())
                    {
                        break;
                    }
                }
            }
            layeredTasks.insert(layeredTasks.end(), layerTask);

            if (temp.empty())
            {
                break;
            }

            if (layerTask.empty())
            {
                throw std::logic_error("circular dependency!");
            }
        }
    }

    std::vector<std::vector<common::OMRendererTask *>> layeredTasks;
    common::OMRendererShaderManager shaderManager;

  protected:
    void *window;
    std::unordered_map<std::string, common::OMRendererTask *> tasks;
    std::vector<std::shared_ptr<common::OMRendererHandler>> handlers;

  private:
    const AppInfo info;
};
} // namespace openminecraft::renderer

#endif
