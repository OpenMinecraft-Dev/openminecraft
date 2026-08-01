#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_image_channel.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"

namespace openminecraft::renderer::common::demiurge::element
{
void OMDemiurgeImageChannel::init(OMRendererBuffer *uniformBuffer, OMRendererRenderTarget *target)
{
    vtxShader = renderer->shaderManager.preprocess("demiurge/image.vert.glsl", Vertex, GLSLSource);
    frgShader = renderer->shaderManager.preprocess("demiurge/image.frag.glsl", Fragment, GLSLSource);

    format.appendPart("position", basics::Vec2f)
        ->nextGroup()
        ->setInstance()
        ->appendPart("image_rrect_pos", basics::Vec4f)
        ->appendPart("image_rrect_color", basics::Vec4f)
        ->appendPart("image_rrect_radius", basics::Vec4f)
        ->appendPart("image_rrect_factor", basics::Float)
        ->appendPart("image_rrect_depth", basics::Float)
        ->appendPart("image_fillmode", basics::Float)
        ->nextGroup()
        ->decideStruct()
        ->debugState();

    quadBuffer = renderer->allocateBuffer(VertexData, 4 * sizeof(glm::vec2));
    quadBuffer->updateData(std::array<glm::vec2, 4>{{{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}}}.data());
    quadIndex = renderer->allocateBuffer(VertexIndex, 6 * sizeof(uint32_t));
    quadIndex->updateData(std::array<uint32_t, 6>{{0, 1, 2, 2, 3, 0}}.data());
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
                                   ->input(ImageSampler)
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

            task->pipeline(pipelines[i])
                ->vertexBuffer({quadBuffer, instanceBuffer})
                ->indexBuffer(quadIndex)
                ->drawInstance(6, 1, i);
        }
    }
}
} // namespace openminecraft::renderer::common::demiurge::element
