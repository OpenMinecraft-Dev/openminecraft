#ifndef OM_RENDERER_LAYER_VK_TESTRENDERER
#define OM_RENDERER_LAYER_VK_TESTRENDERER
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "openminecraft/renderer/common/model/om_renderer_model_obj.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_boxblur.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include <chrono>
#include <functional>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "openminecraft/renderer/common/om_renderer_texture.hpp"

#include <memory>

using namespace openminecraft::renderer::common;

// INFO: The renderer test definition, behaves like a renderer handler (and it should be)
namespace openminecraft::boot::test
{
// INFO: The uniform buffer data structure used for the renderer
// INFO: model, view, proj for object rendering
#pragma pack(1)
struct LightingUniform
{
    glm::vec3 lightDirection;
    float _pad1;
    glm::vec3 lightColor;
    float _pad2;
    glm::vec3 ambientColor;
    float _pad3;
};
#pragma pack()
// INFO: simple vertex structure
using VertexStruct = basics::OMVertex<glm::vec3, glm::vec2>;

class OMTestRenderer : public OMRendererHandler
{
  public:
    OMTestRenderer(renderer::OMRenderer *renderer, std::function<OMRendererTexture *()> t, event::OMEventBusSDL &bus,
                   std::shared_ptr<basics::OMCamera> camera);
    ~OMTestRenderer() override;

    // INFO: count frames per second, submit and create tasks, etc.
    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    // INFO: these are the resource handles used for rendering
    OMRendererBuffer *uniformBuffer, *voxelModelBuffer, *lightingBuffer, *cameraBuffer;
    OMRendererPipeline *mainPipeline;
    wrap::OMRendererTempTarget *tempTargetMS, *tempTarget;
    wrap::OMVoxelManager *voxelManager;

    std::shared_ptr<wrap::OMRendererBoxBlurHandler> blurHandler;

    std::function<OMRendererTexture *()> overlay;

    std::shared_ptr<basics::OMCamera> camera;
    basics::OMVertexFormat format;

    log::OMLogger logger;

  private:
    renderer::OMRenderer *renderer;

    float m_cameraMoveSpeed = 3.4f;

    std::shared_ptr<OMShader> outputVtx;
    std::shared_ptr<OMShader> outputFrg;

    std::chrono::high_resolution_clock::time_point tp;
    bool timing = false;
    int fps = 0;
};
} // namespace openminecraft::boot::test

#endif
