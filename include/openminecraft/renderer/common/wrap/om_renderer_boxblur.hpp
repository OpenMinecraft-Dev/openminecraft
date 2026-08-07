#ifndef OM_RENDERER_BOXBLUR
#define OM_RENDERER_BOXBLUR

#include "glm/ext/vector_float3.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
namespace openminecraft::renderer::common::wrap
{
struct OMRendererBoxBlurArg
{
    float radius;
    float sigma;
};
struct OMRendererBoxBlurVertex
{
    glm::vec3 pos;
    glm::vec2 uv;
};
class OMRendererBoxBlurHandler : public OMRendererHandler
{
  public:
    OMRendererBoxBlurHandler(OMRenderer *);
    ~OMRendererBoxBlurHandler() override;

    void update(OMRendererBoxBlurArg);

    void beforeFrame() override;
    void afterFrame() override;
    void submitTasks() override;

    auto firstLayerTask(OMRendererTask *) -> OMRendererTask *;
    auto secondLayerTask(OMRendererTask *) -> OMRendererTask *;
    void bind(OMRendererTexture *uplayer, OMRendererTexture *bottomLayer);

    OMRendererPipeline *blurp1Pipeline, *blurp2Pipeline;
    OMRendererTempTarget *blurTemp;
    OMRendererBuffer *blurArgs;
    OMRendererBuffer *quadVertex, *quadIndex;

    std::shared_ptr<OMShader> outputBlurp2Vtx, outputBlurp1Vtx;
    std::shared_ptr<OMShader> outputBlurp2Frg, outputBlurp1Frg;

    basics::OMVertexFormat format;

  private:
    OMRenderer *renderer;
};
} // namespace openminecraft::renderer::common::wrap

#endif
