#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_shaderscene_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include <array>
#include <chrono>

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeShaderSceneChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *renderTarget)
{
    format.setInstance()
        ->appendPart("inShaderScenePos", basics::Vec4f)
        ->appendPart("inShaderSceneDepth", basics::Float)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/shaderscene.vert.glsl", Vertex, GLSLSource, format);

    instanceBuffer = renderer->allocateBuffer(InstanceData, 8);

    timeBuffer = renderer->allocateBuffer(Uniform, sizeof(float));
    timeBuffer->updateData(std::array<float, 1>{0.0f}.data());

    this->uniform = uniformBuffer;
    this->target = renderTarget;

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

auto OMDemiurgeShaderSceneChannel::submitTask(OMRendererTask *task, float upper, float lower) -> void
{
    if (!instanceBuffer)
    {
        return;
    }

    if (instanceBuffer->length < this->bufferSize())
    {
        auto *newBuffer = renderer->allocateBuffer(InstanceData, this->bufferSize());

        if (instanceBuffer->length > 0)
        {
            instanceBuffer->copyTo(newBuffer);
        }

        delete instanceBuffer;
        instanceBuffer = newBuffer;
    }

    for (int i = 0; i < static_cast<int>(objects.size()); ++i)
    {
        const auto &obj = objects[i];

        if (obj.depth <= lower || obj.depth >= upper)
        {
            continue;
        }

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

        task->pipeline(pipeline)->vertexBufferInstanced({instanceBuffer}, i)->drawInstance(6, 1, i);
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
                         ->depth(false, false)
                         ->buildN();

    if (uniform)
    {
        pipeline->bindInput(0, uniform);
    }

    if (timeBuffer)
    {
        pipeline->bindInput(1, timeBuffer);
    }

    pipelines.emplace(i, pipeline);
    return pipeline;
}

} // namespace openminecraft::renderer::common::demiurge::element
