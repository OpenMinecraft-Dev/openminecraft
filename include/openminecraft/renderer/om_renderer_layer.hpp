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
#include <functional>
#include <glm/glm.hpp>
#include <mutex>
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
    util::OMVersion appVer;
    std::string engineName;
    util::OMVersion engineVer;
    util::OMVersion minApiVersion;
};
class OMRenderer
{
  public:
    OMRenderer(AppInfo info, void *window, std::string shaderPath);
    virtual ~OMRenderer()
    {
        // Fallback: derived classes normally call clearRegistry() before destroying the device
        clearRegistry();
    }

    // ---- Object registry: all buffers / textures / render targets are owned by the renderer and accessed by string tag ----

    auto registerBuffer(const std::string &tag, common::OMBufferUsage usage, uint64_t length)
        -> common::OMRendererBuffer *
    {
        if (bufferRegistry.count(tag))
        {
            throw std::logic_error("duplicate buffer tag: " + tag);
        }

        auto *buffer = allocateBuffer(usage, length);
        bufferRegistry[tag] = buffer;
        return buffer;
    }

    auto getBuffer(const std::string &tag) -> common::OMRendererBuffer *
    {
        auto it = bufferRegistry.find(tag);

        if (it == bufferRegistry.end())
        {
            throw std::logic_error("buffer tag not found: " + tag);
        }

        return it->second;
    }

    auto hasBuffer(const std::string &tag) const -> bool
    {
        return bufferRegistry.count(tag) != 0;
    }

    void unregisterBuffer(const std::string &tag)
    {
        auto it = bufferRegistry.find(tag);

        if (it == bufferRegistry.end())
        {
            return;
        }

        delete it->second;
        bufferRegistry.erase(it);
    }

    auto registerTexture(const std::string &tag, uint64_t width, uint64_t height, uint64_t mipmap,
                         common::OMTextureType type, common::OMTextureArrangement arrangement)
        -> common::OMRendererTexture *
    {
        return registerTexture(tag, width, height, 1, mipmap, type, arrangement);
    }

    auto registerTexture(const std::string &tag, uint64_t width, uint64_t height, uint64_t layers, uint64_t mipmap,
                         common::OMTextureType type, common::OMTextureArrangement arrangement)
        -> common::OMRendererTexture *
    {
        if (textureRegistry.count(tag))
        {
            throw std::logic_error("duplicate texture tag: " + tag);
        }

        auto *texture = allocateTexture(width, height, layers, mipmap, type, arrangement);
        textureRegistry[tag] = texture;
        return texture;
    }

    auto getTexture(const std::string &tag) -> common::OMRendererTexture *
    {
        auto it = textureRegistry.find(tag);

        if (it == textureRegistry.end())
        {
            throw std::logic_error("texture tag not found: " + tag);
        }

        return it->second;
    }

    auto hasTexture(const std::string &tag) const -> bool
    {
        return textureRegistry.count(tag) != 0;
    }

    void unregisterTexture(const std::string &tag)
    {
        auto it = textureRegistry.find(tag);

        if (it == textureRegistry.end())
        {
            return;
        }

        delete it->second;
        textureRegistry.erase(it);
    }

    auto registerRenderTarget(const std::string &tag) -> common::OMRendererRenderTarget *
    {
        if (renderTargetRegistry.count(tag))
        {
            throw std::logic_error("duplicate render target tag: " + tag);
        }

        auto *target = createRenderTarget();
        renderTargetRegistry[tag] = target;
        return target;
    }

    auto getRenderTarget(const std::string &tag) -> common::OMRendererRenderTarget *
    {
        auto it = renderTargetRegistry.find(tag);

        if (it == renderTargetRegistry.end())
        {
            throw std::logic_error("render target tag not found: " + tag);
        }

        return it->second;
    }

    auto hasRenderTarget(const std::string &tag) const -> bool
    {
        return renderTargetRegistry.count(tag) != 0;
    }

    void unregisterRenderTarget(const std::string &tag)
    {
        auto it = renderTargetRegistry.find(tag);

        if (it == renderTargetRegistry.end())
        {
            return;
        }

        delete it->second;
        renderTargetRegistry.erase(it);
    }

    // Must be called before the device is destroyed (at the start of derived destructors)
    void clearRegistry()
    {
        for (auto &p : bufferRegistry)
        {
            delete p.second;
        }
        bufferRegistry.clear();

        for (auto &p : textureRegistry)
        {
            delete p.second;
        }
        textureRegistry.clear();

        for (auto &p : renderTargetRegistry)
        {
            delete p.second;
        }
        renderTargetRegistry.clear();
    }

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
        if (tasks.count(id))
        {
            return tasks[id];
        }
        return nullptr;
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

    virtual auto fetchFrameTime() -> float = 0;

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

    void addSchedule(std::function<void()> f)
    {
        std::lock_guard g(scheduleMutex);
        scheduleQueue.push_back(f);
    }

  protected:
    void *window;
    std::mutex scheduleMutex = {};
    std::unordered_map<std::string, common::OMRendererTask *> tasks;
    std::vector<std::shared_ptr<common::OMRendererHandler>> handlers;

    std::vector<std::function<void()>> scheduleQueue;

    std::unordered_map<std::string, common::OMRendererBuffer *> bufferRegistry;
    std::unordered_map<std::string, common::OMRendererTexture *> textureRegistry;
    std::unordered_map<std::string, common::OMRendererRenderTarget *> renderTargetRegistry;
    void runSchedules()
    {
        std::lock_guard g(scheduleMutex);
        for (auto &p : scheduleQueue)
        {
            p();
        }
    }

  private:
    const AppInfo info;
};
} // namespace openminecraft::renderer

#endif
