#ifndef OM_DEMIURGE_RENDERERHANDLER_HPP
#define OM_DEMIURGE_RENDERERHANDLER_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
namespace openminecraft::renderer::common::demiurge
{
class OMDemiurgeRendererHandler : public OMRendererHandler
{
  public:
    OMDemiurgeRendererHandler(OMRenderer *renderer);
    ~OMDemiurgeRendererHandler() override;

    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    std::shared_ptr<OMDemiurgeNode> node;

    std::shared_ptr<OMShader> vtxShader, frgShader;
    OMRendererPipeline *uiPipeline;
    OMRendererBuffer *uniformBuffer;
    OMRenderer *renderer;

  private:
    basics::OMVertexFormat format;
};
} // namespace openminecraft::renderer::common::demiurge

#endif
