#ifndef OM_DEMIURGE_ELEMENT_SHADERSCENE_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_SHADERSCENE_CHANNEL_HPP

#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_shaderscene.hpp"
#include <chrono>
#include <functional>
#include <string>
#include <unordered_map>

namespace openminecraft::renderer::common::demiurge::element
{
// INFO: channel that draws a full-screen scene with a fragment shader.
// Each element can bind its own fragment shader (selected by shaders[i]),
// the shader requires two uniform blocks:
//   ScreenData { float width; float height; }   -- provided by the uniform buffer passed to init
//   Time       { float value; }                 -- time updated internally every frame
// See bootassets/openminecraft-renderer/shaders/demiurge/scene/scene*.frag.glsl
class OMDemiurgeShaderSceneChannel : public OMDemiurgeQuadChannel<OMDemiurgeElementShaderScene>
{
  public:
    OMDemiurgeShaderSceneChannel(OMRenderer *renderer, std::function<void()> f) : OMDemiurgeQuadChannel(renderer, f)
    {
    }
    ~OMDemiurgeShaderSceneChannel() = default;

    void init(const std::string &uniformTag, const std::string &targetTag) override;
    auto submitTask(OMRendererTask *task, float upper, float lower) -> void override;
    auto update() -> void override;

    auto destroy() -> void override
    {
        if (timeBuffer)
        {
            renderer->unregisterBuffer(timeBuffer.getTag());
            timeBuffer = wrap::OMBufferRef();
        }

        for (auto p : pipelines)
        {
            delete p.second;
        }
        pipelines.clear();
    }

    auto onRemove(int i) -> void override
    {
        auto it = pipelines.find(i);
        if (it != pipelines.end())
        {
            delete it->second;
            pipelines.erase(it);
        }
        shaders.erase(i);
    }

    // Bind a fragment shader to element i.
    // Pass either a full relative path (demiurge/scene/scene1.frag.glsl)
    // or just a name (scene1); it is expanded internally.
    auto bindShader(int i, const std::string &name) -> void
    {
        shaders[i] = name;

        auto it = pipelines.find(i);
        if (it != pipelines.end())
        {
            delete it->second;
            pipelines.erase(it);
        }

        ensurePipeline(i);
    }

  private:
    auto ensurePipeline(int i) -> OMRendererPipeline *;

    auto resolveShaderName(const std::string &name) const -> std::string
    {
        if (name.find('/') != std::string::npos)
        {
            return name;
        }

        std::string full = "demiurge/scene/" + name;
        if (full.size() < 10 || full.compare(full.size() - 10, 10, ".frag.glsl") != 0)
        {
            full += ".frag.glsl";
        }
        return full;
    }

    wrap::OMBufferRef uniform;
    wrap::OMRenderTargetRef target;
    wrap::OMBufferRef timeBuffer;
    std::shared_ptr<OMShader> vtxShader;

    std::chrono::steady_clock::time_point startedAt = std::chrono::steady_clock::now();

    std::unordered_map<int, std::string> shaders;
    std::unordered_map<int, OMRendererPipeline *> pipelines;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
