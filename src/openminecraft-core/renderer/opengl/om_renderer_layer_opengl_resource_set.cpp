#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_resource_set.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"

using namespace openminecraft::renderer::common;

namespace openminecraft::renderer::opengl
{
OMRendererResourceSetOpenGL::OMRendererResourceSetOpenGL(OMRendererOpenGL *renderer, OMRendererPipelineOpenGL *pipeline)
    : OMRendererResourceSet(renderer, pipeline), glPipeline(pipeline)
{
    bound.resize(pipeline->inputTypes.size(), nullptr);
}

void OMRendererResourceSetOpenGL::bindInput(int index, OMRendererBuffer *buffer)
{
    if (index < 0)
    {
        return;
    }
    if (static_cast<size_t>(index) >= bound.size())
    {
        bound.resize(static_cast<size_t>(index) + 1, nullptr);
    }
    bound[static_cast<size_t>(index)] = buffer;
}

void OMRendererResourceSetOpenGL::bindInput(int index, OMRendererTexture *texture)
{
    if (index < 0)
    {
        return;
    }
    if (static_cast<size_t>(index) >= bound.size())
    {
        bound.resize(static_cast<size_t>(index) + 1, nullptr);
    }
    bound[static_cast<size_t>(index)] = texture;
}

void OMRendererResourceSetOpenGL::bindInputName(std::string)
{
}
} // namespace openminecraft::renderer::opengl
