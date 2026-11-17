#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_shaderscene_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include <array>
#include <chrono>

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeShaderSceneChannel::init(const std::string &uniformTag, const std::string &targetTag)
{
    uniform = openminecraft::renderer::common::OMRendererBufferRef(renderer, uniformTag);
    target = openminecraft::renderer::common::OMRendererRenderTargetRef(renderer, targetTag);

    format.setInstance()
        ->appendPart("inShaderScenePos", basics::Vec4f)
        ->appendPart("inShaderSceneDepth", basics::UInteger)
        ->appendPart("inShaderSceneScale", basics::Vec2f)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/shaderscene.vert.glsl", Vertex, GLSLSource, format);

    resetInstanceBuffer(8);

    timeBuffer = openminecraft::renderer::common::OMRendererBufferRef(renderer, instanceTag + ".time");
    renderer->registerBuffer(timeBuffer.getTag(), Uniform, sizeof(float));
    timeBuffer->updateData(std::array<float, 1>{0.0f}.data());


    startedAt = std::chrono::steady_clock::now();
}

auto OMDemiurgeShaderSceneChannel::update() -> void
{
    OMDemiurgeQuadChannel<OMDemiurgeElementShaderScene>::update();

    if (timeBuffer)
    {
        float value =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt).count();
        timeBuffer->updateData(std::array<float, 1>{value / 1000.0f}.data());
    }
}

auto OMDemiurgeShaderSceneChannel::submitTask(OMRendererTask *task, uint32_t layer) -> void
{
    if (!instanceBuffer)
    {
        return;
    }

    if (instanceBuffer->length < this->bufferSize())
    {
        const std::string newTag = instanceTag + ".g" + std::to_string(++instanceGeneration);
        auto newBuffer = openminecraft::renderer::common::OMRendererBufferRef(renderer, newTag);
        renderer->registerBuffer(newTag, InstanceData, this->bufferSize());

        if (instanceBuffer->length > 0)
        {
            instanceBuffer->copyTo(newBuffer);
        }

        renderer->unregisterBuffer(instanceBuffer.getTag());
        instanceBuffer = newBuffer;
    }

    for (int i : this->layerDrawOrder(layer))
    {
        auto shaderIt = shaders.find(i);
        if (shaderIt == shaders.end())
        {
            continue;
        }

        auto *pipeline = ensurePipeline(i);

        if (!pipeline)
        {
            continue;
        }

        applyClipScissor(task, i);
        task->pipeline(pipeline)->resourceSet(resourceSets[i])->vertexBufferInstanced({instanceBuffer}, i)->drawInstance(6, 1, i);
    }
}

auto OMDemiurgeShaderSceneChannel::ensurePipeline(int i) -> OMRendererPipeline *
{
    auto it = pipelines.find(i);
    if (it != pipelines.end())
    {
        return it->second;
    }

    auto shaderIt = shaders.find(i);
    if (shaderIt == shaders.end() || !renderer || !target)
    {
        return nullptr;
    }

    auto frgShader =
        renderer->shaderManager.preprocess(resolveShaderName(shaderIt->second), Fragment, GLSLSource, format);

    auto *pipeline = renderer->createPipeline()
                         ->input(UniformBuffer)
                         ->inputName("ScreenData")
                         ->input(UniformBuffer)
                         ->inputName("Time")
                         ->output(target)
                         ->shader(frgShader)
                         ->shader(vtxShader)
                         ->format(format)
                         ->blendFunc({SrcAlpha, OneMinusSrcAlpha, One, OneMinusSrcAlpha})
                         ->blend(false)
                         ->depth(true, true)
                         ->depthOp(LessOrEqual)
                         ->buildN();

    auto *set = pipeline->createResourceSet();

    if (uniform)
    {
        set->bindInput(0, uniform);
    }

    if (timeBuffer)
    {
        set->bindInput(1, timeBuffer);
    }

    resourceSets.emplace(i, set);
    pipelines.emplace(i, pipeline);
    return pipeline;
}

} // namespace openminecraft::renderer::common::demiurge::element
