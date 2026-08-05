#ifndef OM_RENDERER_LAYER_VK_TESTRENDERER
#define OM_RENDERER_LAYER_VK_TESTRENDERER
#include "glm/fwd.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
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
// INFO: kernelSize, sigma for post-processing
// TODO: separate the two parts
struct UniformStructure
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    float kernelSize;
    float sigma;
};
// INFO: simple vertex structure
struct VertexStruct
{
    glm::vec3 pos;
    glm::vec2 uv;
};

class OMTestRenderer : public OMRendererHandler
{
  public:
    OMTestRenderer(renderer::OMRenderer *renderer, std::function<OMRendererTexture *()> t, event::OMEventBusSDL &bus);
    ~OMTestRenderer() override;

    // INFO: count frames per second, submit and create tasks, etc.
    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    // INFO: these are the resource handles used for rendering
    OMRendererBuffer *vertexBuffer;
    OMRendererBuffer *indexBuffer;
    OMRendererBuffer *uniformBuffer;
    OMRendererBuffer *tempUniformBuffer;
    OMRendererTexture *textureImage;
    OMRendererPipeline *pipeline = nullptr;
    OMRendererBuffer *mainVtxBuffer;
    OMRendererBuffer *mainIdxBuffer;
    OMRendererPipeline *mainPipeline;
    OMRendererPipeline *mainPipeline2;
    OMRendererPipeline *mainPipeline3 = nullptr;
    wrap::OMRendererTempTarget *tempTarget;
    wrap::OMRendererTempTarget *blurTemp;
    OMRendererBuffer *blurArgs;

    std::function<OMRendererTexture *()> overlay;

    // INFO: event handling, runs on a different thread
    void keyInput(bool w, bool a, bool s, bool d, bool lsh, bool sp);
    void mouseOffset(float dx, float dy);
    std::shared_ptr<basics::OMCamera> camera;
    basics::OMVertexFormat format;

    log::OMLogger logger;

  private:
    int vertexCount = 0;
    renderer::OMRenderer *renderer;

    glm::vec3 m_cameraPos{2.f, 2.0f, 2.f};
    glm::vec3 m_cameraUp{0.0f, 1.0f, 0.0f};
    float m_pitch = -35.0f;
    float m_yaw = -135.0f;
    float m_cameraMoveSpeed = 2.0f;
    float m_cameraRotateSpeed = 45.0f;

    std::shared_ptr<OMShader> objectVtx;
    std::shared_ptr<OMShader> objectFrg;
    std::shared_ptr<OMShader> outputVtx, outputVtx2, outputVtx3;
    std::shared_ptr<OMShader> outputFrg, outputFrg2, outputFrg3;

    std::chrono::high_resolution_clock::time_point tp;
    bool timing = false;
    int fps = 0;
};
} // namespace openminecraft::boot::test

#endif
