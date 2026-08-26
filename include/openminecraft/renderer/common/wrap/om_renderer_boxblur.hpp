#ifndef OM_RENDERER_BOXBLUR
#define OM_RENDERER_BOXBLUR

#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
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
    float direction;
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

    auto blurPass(OMRendererTask *) -> OMRendererTask *;
    auto firstLayerTask(OMRendererTask *) -> OMRendererTask *;
    auto secondLayerTask(OMRendererTask *) -> OMRendererTask *;
    void bind(OMRendererTexture *uplayer, OMRendererTexture *bottomLayer);

    OMRendererPipeline *biltPipeline, *blurp1Pipeline, *blurp2Pipeline, *composePipeline;
    OMRendererTempTarget *blurTemp, *blurTemp2;
    OMRendererBuffer *blurArgs, *blurArgs2;

    basics::OMVertexFormat format;

  private:
    OMRenderer *renderer;
};
} // namespace openminecraft::renderer::common::wrap

#endif
