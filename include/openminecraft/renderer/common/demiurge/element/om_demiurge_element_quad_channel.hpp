#ifndef OM_DEMIURGE_ELEMENT_QUAD_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_QUAD_CHANNEL_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_object_ref.hpp"
#include <functional>
#include <iostream>
#include <utility>
namespace openminecraft::renderer::common::demiurge::element
{
template <typename T> class OMDemiurgeQuadChannel : public OMDemiurgeChannel<T>
{
  public:
    OMDemiurgeQuadChannel(OMRenderer *renderer, std::function<void()> recreate)
        : renderer(renderer), recreation(std::move(recreate))
    {
        instanceTag = "demiurge.quad." + std::to_string(wrap::nextObjectTagId());
    }
    ~OMDemiurgeQuadChannel() = default;

    auto destroy() -> void override
    {
        if (instanceBuffer)
        {
            renderer->unregisterBuffer(instanceBuffer.getTag());
            instanceBuffer = wrap::OMBufferRef();
        }
        delete pipeline;
    }

    auto init(const std::string &uniformTag, const std::string &targetTag) -> void override
    {
        initPipeline(wrap::OMBufferRef(renderer, uniformTag), wrap::OMRenderTargetRef(renderer, targetTag), true);
    }

    auto submitTask(OMRendererTask *task, float upper, float lower) -> void override
    {
        if (instanceBuffer->length < this->bufferSize())
        {
            const std::string newTag = instanceTag + ".g" + std::to_string(++instanceGeneration);
            auto ninstanceBuffer = wrap::OMBufferRef(renderer, newTag);
            renderer->registerBuffer(newTag, InstanceData, this->bufferSize());
            instanceBuffer->copyTo(ninstanceBuffer);

            renderer->unregisterBuffer(instanceBuffer.getTag());
            instanceBuffer = ninstanceBuffer;
        }

        bool inDraw = false;
        int start = 0;
        for (int i = 0; i <= this->objects.size(); ++i)
        {
            bool isDraw =
                (i < this->objects.size()) && this->objects[i].depth > lower && this->objects[i].depth < upper;
            if (!inDraw && isDraw)
            {
                start = i;
                inDraw = true;
            }
            else if (inDraw && !isDraw)
            {
                task->pipeline(pipeline)
                    ->vertexBufferInstanced({instanceBuffer}, start)
                    ->drawInstance(6, i - start, start);
                inDraw = false;
            }
        }
    }

    auto update() -> void override
    {
        if (instanceBuffer->length < this->bufferSize() || this->lastCount != this->objects.size())
        {
            this->lastCount = this->objects.size();
            recreation();
            return;
        }

        this->dirtyIter([&](int begin, int length) -> void {
            instanceBuffer->updateDataPart(&this->objects[begin], begin * sizeof(T), length * sizeof(T));
        });
    }

  protected:
    // Shared pipeline construction for all Demiurge quad channels; with writeColor = false the channel only writes depth,
    // producing no writes to the color attachment (used by clipping-only channels such as cliprect).
    // Reallocate the instance buffer (the old object is unregistered from the registry)
    auto resetInstanceBuffer(uint64_t size) -> void
    {
        if (instanceBuffer)
        {
            renderer->unregisterBuffer(instanceBuffer.getTag());
        }

        const std::string tag = instanceTag + ".g" + std::to_string(++instanceGeneration);
        instanceBuffer = wrap::OMBufferRef(renderer, tag);
        renderer->registerBuffer(tag, InstanceData, size);
    }

    auto initPipeline(OMRendererBuffer *uniform, OMRendererRenderTarget *target, bool writeColor) -> void
    {
        resetInstanceBuffer(8);

        pipeline = renderer->createPipeline()
                       ->input(UniformBuffer)
                       ->inputName("ScreenData")
                       ->output(target)
                       ->shader(frgShader)
                       ->shader(vtxShader)
                       ->format(format)
                       ->blendFunc({SrcAlpha, OneMinusSrcAlpha, One, OneMinusSrcAlpha})
                       ->blend(true)
                       ->depth(true, true)
                       ->depthOp(LessOrEqual)
                       ->writeColor(writeColor)
                       ->buildN();
        pipeline->bindInput(0, uniform);
    }

    OMRenderer *renderer;
    std::function<void()> recreation;
    basics::OMVertexFormat format;

    wrap::OMBufferRef instanceBuffer;
    std::string instanceTag;
    uint64_t instanceGeneration = 0;

    OMRendererPipeline *pipeline;
    std::shared_ptr<OMShader> vtxShader, frgShader;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
