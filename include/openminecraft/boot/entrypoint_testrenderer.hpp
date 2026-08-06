#ifndef OM_RENDERER_LAYER_VK_TESTRENDERER
#define OM_RENDERER_LAYER_VK_TESTRENDERER
#include "glm/ext/vector_float3.hpp"
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
#include "openminecraft/renderer/common/wrap/om_renderer_boxblur.hpp"
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
#pragma pack(1)
struct UniformStructure
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 lightDirection;
    float _pad1;
    glm::vec3 lightColor;
    float _pad2;
    glm::vec3 ambientColor;
    float _pad3;
};
#pragma pack()
// INFO: simple vertex structure
struct VertexStruct
{
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
    auto operator==(const VertexStruct &other) const -> bool
    {
        return pos == other.pos && uv == other.uv & normal == other.normal;
    }
};

struct VertexHash
{
    auto operator()(const VertexStruct &v) const -> std::size_t
    {
        size_t h1 = std::hash<float>{}(v.pos.x);
        size_t h2 = std::hash<float>{}(v.pos.y);
        size_t h3 = std::hash<float>{}(v.pos.z);
        size_t h4 = std::hash<float>{}(v.uv.x);
        size_t h5 = std::hash<float>{}(v.uv.y);
        size_t h6 = std::hash<float>{}(v.normal.x);
        size_t h7 = std::hash<float>{}(v.normal.y);
        size_t h8 = std::hash<float>{}(v.normal.z);
        return ((((((h1 * 31 + h2) * 31 + h3) * 31 + h4) * 31 + h5) * 31 + h6) * 31 + h7) * 31 + h8;
    }
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
    wrap::OMRendererTempTarget *tempTarget;

    std::shared_ptr<wrap::OMRendererBoxBlurHandler> blurHandler;

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

    float m_cameraMoveSpeed = 2.0f;
    float m_cameraRotateSpeed = 45.0f;

    std::shared_ptr<OMShader> objectVtx;
    std::shared_ptr<OMShader> objectFrg;
    std::shared_ptr<OMShader> outputVtx;
    std::shared_ptr<OMShader> outputFrg;

    std::chrono::high_resolution_clock::time_point tp;
    bool timing = false;
    int fps = 0;
};
} // namespace openminecraft::boot::test

#endif
