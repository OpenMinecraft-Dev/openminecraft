#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_image_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeImageChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *target)
{
    format.setInstance()
        ->appendPart("inRectPos", basics::Vec4f)
        ->appendPart("inRectColor", basics::Vec4f)
        ->appendPart("inRectRadius", basics::Vec4f)
        ->appendPart("inRectRotation", basics::Vec4f)
        ->appendPart("inRectFactor", basics::Float)
        ->appendPart("inRectDepth", basics::Float)
        ->appendPart("inFillType", basics::Float)
        ->nextGroup()
        ->decideStruct();

    vtxShader = renderer->shaderManager.preprocess("demiurge/image.vert.glsl", Vertex, GLSLSource, format);
    frgShader = renderer->shaderManager.preprocess("demiurge/image.frag.glsl", Fragment, GLSLSource, format);

    instanceBuffer = renderer->allocateBuffer(InstanceData, 8);

    this->uniform = uniformBuffer;
    this->target = target;
}

auto OMDemiurgeImageChannel::submitTask(OMRendererTask *task, float upper, float lower,
                                        OMDemiurgeAbstractChannel *&currentChannel) -> void
{
    if (instanceBuffer->length < this->bufferSize())
    {
        delete instanceBuffer;
        instanceBuffer = renderer->allocateBuffer(InstanceData, this->bufferSize());
    }

    for (int i = 0; i < objects.size(); ++i)
    {
        if (this->objects[i].depth > lower && this->objects[i].depth < upper)
        {
            if (!textures.count(i))
            {
                continue;
            }

            if (!pipelines.count(i))
            {
                pipelines[i] = renderer->createPipeline()
                                   ->input(UniformBuffer)
                                   ->inputName("ScreenData")
                                   ->input(ImageSampler)
                                   ->inputName("inTexture")
                                   ->output(target)
                                   ->shader(frgShader)
                                   ->shader(vtxShader)
                                   ->format(format)
                                   ->blendFunc({Alpha, OneMinusAlpha, One, OneMinusAlpha})
                                   ->blend(true)
                                   ->depth(false, false)
                                   ->buildN();
                pipelines[i]->bindInput(0, uniform);
                pipelines[i]->bindInput(1, textures[i]);
            }

            task->pipeline(pipelines[i])->vertexBufferInstanced({instanceBuffer}, i)->drawInstance(6, 1, i);
            currentChannel = this;
        }
    }
}
} // namespace openminecraft::renderer::common::demiurge::element
