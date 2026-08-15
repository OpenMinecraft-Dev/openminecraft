#ifndef OM_DEMIURGE_ELEMENT_QUAD_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_QUAD_CHANNEL_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_channel.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include <functional>
#include <utility>
namespace openminecraft::renderer::common::demiurge::element
{
template <typename T> class OMDemiurgeQuadChannel : public OMDemiurgeChannel<T>
{
  public:
    OMDemiurgeQuadChannel(OMRenderer *renderer, std::function<void()> recreate)
        : renderer(renderer), recreation(std::move(recreate))
    {
    }
    ~OMDemiurgeQuadChannel() = default;

    auto destroy() -> void override
    {
        delete instanceBuffer;
        delete pipeline;
    }

    auto init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) -> void override
    {
        instanceBuffer = renderer->allocateBuffer(InstanceData, 8);

        pipeline = renderer->createPipeline()
                       ->input(UniformBuffer)
                       ->inputName("ScreenData")
                       ->output(target)
                       ->shader(frgShader)
                       ->shader(vtxShader)
                       ->format(format)
                       ->blendFunc({Alpha, OneMinusAlpha, One, OneMinusAlpha})
                       ->blend(true)
                       ->depth(false, false)
                       ->buildN();
        pipeline->bindInput(0, uniform);
    }

    auto submitTask(OMRendererTask *task, float upper, float lower, OMDemiurgeAbstractChannel *&currentChannel)
        -> void override
    {
        if (instanceBuffer->length < this->bufferSize())
        {
            auto ninstanceBuffer = renderer->allocateBuffer(InstanceData, this->bufferSize());
            instanceBuffer->copyTo(ninstanceBuffer);

            delete instanceBuffer;
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
                if (currentChannel != this)
                {
                    task->pipeline(pipeline)->vertexBufferInstanced({instanceBuffer}, start);
                    currentChannel = this;
                }
                task->drawInstance(6, i - start, start);
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
    OMRenderer *renderer;
    std::function<void()> recreation;
    basics::OMVertexFormat format;

    OMRendererBuffer *instanceBuffer = nullptr;

    OMRendererPipeline *pipeline;
    std::shared_ptr<OMShader> vtxShader, frgShader;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
