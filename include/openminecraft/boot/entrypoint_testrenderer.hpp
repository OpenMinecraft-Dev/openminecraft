#ifndef OM_RENDERER_LAYER_VK_TESTRENDERER
#define OM_RENDERER_LAYER_VK_TESTRENDERER
#include "glm/fwd.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"

#include <memory>

using namespace openminecraft::renderer::common;

namespace openminecraft::boot::test
{
struct UniformStructure
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    float kernelSize;
    float sigma;
};

class OMTestRenderer : public OMRendererHandler
{
  public:
    OMTestRenderer(renderer::OMRenderer *renderer);
    ~OMTestRenderer() override;

    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    OMRendererBuffer *vertexBuffer;
    OMRendererBuffer *indexBuffer;
    OMRendererBuffer *uniformBuffer;
    OMRendererBuffer *tempUniformBuffer;
    OMRendererTexture *textureImage;
    OMRendererPipeline *pipeline;

    OMRendererBuffer *mainVtxBuffer;
    OMRendererBuffer *mainIdxBuffer;

    OMRendererPipeline *mainPipeline;

    OMRendererTexture *tempTexture;
    OMRendererTexture *tempDepth;
    OMRendererRenderTarget *renderTarget;

    void keyInput(bool w, bool a, bool s, bool d, bool lsh, bool sp);
    void mouseOffset(float dx, float dy);

    std::shared_ptr<basics::OMCamera> camera;

    log::OMLogger logger;

  private:
    bool firstTime = true;
    int vertexCount = 0;
    renderer::OMRenderer *renderer;

    glm::vec3 m_cameraPos{2.f, 2.0f, 2.f};
    glm::vec3 m_cameraUp{0.0f, 1.0f, 0.0f};
    float m_pitch = -35.0f;
    float m_yaw = -135.0f;
    float m_cameraMoveSpeed = 2.0f;
    float m_cameraRotateSpeed = 45.0f;

    std::shared_ptr<OMShader> vtxShader;
    std::shared_ptr<OMShader> frgShader;
    std::shared_ptr<OMShader> frgShader2;
};
} // namespace openminecraft::boot::test

#endif
