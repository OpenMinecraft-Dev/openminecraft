#ifndef OM_RENDERER_RESOURCE_SET_HPP
#define OM_RENDERER_RESOURCE_SET_HPP

#include "openminecraft/renderer/om_renderer_object.hpp"
#include <string>

namespace openminecraft::renderer
{
class OMRenderer;
}

namespace openminecraft::renderer::common
{
class OMRendererBuffer;
class OMRendererPipeline;
class OMRendererTexture;

// Backend-neutral resource bindings for one pipeline layout. Pipeline state
// (program, shader modules, vertex layout) and the resources consumed by that
// program are deliberately separate objects.
class OMRendererResourceSet : public OMRendererObject
{
  public:
    OMRendererResourceSet(OMRenderer *renderer, OMRendererPipeline *pipeline)
        : renderer(renderer), pipelineObject(pipeline)
    {
    }
    ~OMRendererResourceSet() override = default;

    virtual void bindInput(int index, OMRendererBuffer *buffer) = 0;
    virtual void bindInput(int index, OMRendererTexture *texture) = 0;
    virtual void bindInputName(std::string name) = 0;

    auto pipeline() const -> OMRendererPipeline *
    {
        return pipelineObject;
    }

    auto objType() -> OMRendererObjectType override
    {
        return ResourceSet;
    }

  protected:
    OMRenderer *renderer;
    OMRendererPipeline *pipelineObject;
};
} // namespace openminecraft::renderer::common

#endif
