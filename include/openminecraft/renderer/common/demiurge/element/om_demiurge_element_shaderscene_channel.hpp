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
// INFO: 用 fragment shader 绘制整屏场景的通道。
// 每个元素可以绑定自己的 fragment shader（由 shaders[i] 指定），
// shader 需要两个 uniform block：
//   ScreenData { float width; float height; }   —— 由 init 传入的 uniform buffer 提供
//   Time       { float value; }                 —— 通道内部每帧更新的时间
// 参考 bootassets/openminecraft-renderer/shaders/demiurge/scene/scene*.frag.glsl
class OMDemiurgeShaderSceneChannel : public OMDemiurgeQuadChannel<OMDemiurgeElementShaderScene>
{
  public:
    OMDemiurgeShaderSceneChannel(OMRenderer *renderer, std::function<void()> f) : OMDemiurgeQuadChannel(renderer, f)
    {
    }
    ~OMDemiurgeShaderSceneChannel() = default;

    void init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) override;
    auto submitTask(OMRendererTask *task, float upper, float lower) -> void override;
    auto update() -> void override;

    auto destroy() -> void override
    {
        delete instanceBuffer;
        delete timeBuffer;

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

    // 给第 i 个元素绑定 fragment shader。
    // 既可以传完整相对路径（demiurge/scene/scene1.frag.glsl），
    // 也可以只传名字（scene1），内部会自动补全。
    auto bindShader(int i, const std::string &name) -> void
    {
        shaders[i] = name;

        auto it = pipelines.find(i);
        if (it != pipelines.end())
        {
            delete it->second;
            pipelines.erase(it);
        }
    }

  private:
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

    OMRendererBuffer *uniform = nullptr;
    OMRendererRenderTarget *target = nullptr;
    OMRendererBuffer *timeBuffer = nullptr;
    std::shared_ptr<OMShader> vtxShader;

    std::chrono::steady_clock::time_point startedAt = std::chrono::steady_clock::now();

    std::unordered_map<int, std::string> shaders;
    std::unordered_map<int, OMRendererPipeline *> pipelines;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
