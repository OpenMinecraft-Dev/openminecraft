#ifndef OM_RENDERER_BLUR_HPP
#define OM_RENDERER_BLUR_HPP

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
enum OMRendererBlurType : int
{
    Gaussian = 0,
    Box = 1,
    Triangular = 2,
    SmoothStep = 3,
    Radial = 4
};
struct OMRendererBlurArg
{
    float radius;
    int blurType;
    float direction;
};
class OMRendererBlurHandler : public OMRendererHandler
{
  public:
    OMRendererBlurHandler(OMRenderer *, int);
    ~OMRendererBlurHandler() override;

    void update(OMRendererBlurArg);

    void beforeFrame() override;
    void afterFrame() override;
    void submitTasks() override;

    auto firstLayerTask(OMRendererTask *) -> OMRendererTask *;
    auto secondLayerTask(OMRendererTask *) -> OMRendererTask *;
    void bind(OMRendererTexture *uplayer, OMRendererTexture *bottomLayer);

    OMRendererPipeline *biltPipeline, *blurp1Pipeline, *blurp2Pipeline, *composePipeline;
    OMRendererTempTarget *blurTemp, *blurTemp2;
    OMRendererBuffer *blurArgs, *blurArgs2;

    basics::OMVertexFormat format;

  private:
    OMRenderer *renderer;
    int passes = 1;
};
} // namespace openminecraft::renderer::common::wrap

#endif
