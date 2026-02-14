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

namespace openminecraft::renderer::test
{
struct UniformStructure
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    float kernelSize;
    float sigma;
};

class OMTestRenderer : public common::OMRendererHandler
{
  public:
    OMTestRenderer(OMRenderer *renderer);
    ~OMTestRenderer() override;

    void onResize() override;
    void beforeFrame() override;
    void afterFrame() override;

    common::OMRendererBuffer *vertexBuffer;
    common::OMRendererBuffer *indexBuffer;
    common::OMRendererBuffer *uniformBuffer;
    common::OMRendererBuffer *tempUniformBuffer;
    common::OMRendererTexture *textureImage;
    common::OMRendererPipeline *pipeline;

    common::OMRendererBuffer *mainVtxBuffer;
    common::OMRendererBuffer *mainIdxBuffer;

    common::OMRendererPipeline *mainPipeline;

    common::OMRendererTexture *tempTexture;
    common::OMRendererTexture *tempDepth;
    common::OMRendererRenderTarget *renderTarget;

    void keyInput(bool w, bool a, bool s, bool d, bool lsh, bool sp);
    void mouseOffset(float dx, float dy);

    std::shared_ptr<common::basics::OMCamera> camera;

    log::OMLogger logger;

  private:
    bool firstTime = true;
    int vertexCount = 0;
    OMRenderer *renderer;

    glm::vec3 m_cameraPos{2.f, 2.0f, 2.f};
    glm::vec3 m_cameraUp{0.0f, 1.0f, 0.0f};
    float m_pitch = -35.0f;
    float m_yaw = -135.0f;
    float m_cameraMoveSpeed = 2.0f;
    float m_cameraRotateSpeed = 45.0f;

    std::shared_ptr<common::OMShader> vtxShader;
    std::shared_ptr<common::OMShader> frgShader;
    std::shared_ptr<common::OMShader> frgShader2;
};
} // namespace openminecraft::renderer::test

#endif
