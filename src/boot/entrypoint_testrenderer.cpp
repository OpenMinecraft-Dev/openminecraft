#include "openminecraft/boot/entrypoint_testrenderer.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "openminecraft/renderer/common/model/om_renderer_model_obj.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_boxblur.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

#include <chrono>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include "tiny_obj_loader.h"

using namespace openminecraft::renderer::common;

namespace openminecraft::boot::test
{
OMTestRenderer::OMTestRenderer(renderer::OMRenderer *renderer, std::function<OMRendererTexture *()> overlay,
                               event::OMEventBusSDL &bus, std::shared_ptr<basics::OMCamera> camera)
    : renderer(renderer), logger("OMTestRenderer", this), OMRendererHandler(renderer), camera(camera)
{
    this->overlay = overlay;
    camera = std::make_shared<basics::OMCamera>(renderer, glm::vec3{2.0f, 2.0f, 2.0f}, -135.0f, -35.0f);

    format.appendPart("inPosition", basics::Vec3f)
        ->appendPart("inTexCoord", basics::Vec2f)
        ->nextGroup()
        ->decideStruct();

    objModel = new model::OMRendererModelObj(
        renderer, vfs::fsfetch("/bootassets/openminecraft-renderer/models/viking_room.obj").get());

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(UniformStructure));

    auto imgraw = vfs::fsfetch("/bootassets/openminecraft-renderer/texture/viking_room.png");

    specs::png::OMPngFile img;
    img.parse(imgraw);

    textureImage = renderer->allocateTexture(img.getWidth(), img.getHeight(), 2, 4, Dim2Array, ColorRgba);
    textureImage->updateData(img.fetchData(), 0);
    textureImage->updateData(img.fetchData(), 1);
    textureImage->setupSampler();

    objectFrg = renderer->shaderManager.preprocess("core/objectbase.frag.glsl", Fragment, GLSLSource, objModel->format);
    objectVtx = renderer->shaderManager.preprocess("core/objectbase.vert.glsl", Vertex, GLSLSource, objModel->format);
    outputFrg = renderer->shaderManager.preprocess("core/bilt.frag.glsl", Fragment, GLSLSource, format);
    outputVtx = renderer->shaderManager.preprocess("core/bilt.vert.glsl", Vertex, GLSLSource, format);

    // INFO: core pipeline creation
    mainPipeline = renderer->createPipeline()
                       ->input(ImageSampler)
                       ->inputName("inTexture")
                       ->output(renderer->getDefaultRenderTarget())
                       ->shader(outputFrg)
                       ->shader(outputVtx)
                       ->format(format)
                       ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                       ->blend(true)
                       ->depth(false, false)
                       ->buildN();

    blurHandler = std::make_shared<wrap::OMRendererBoxBlurHandler>(renderer);
    renderer->registerHandler(blurHandler);
    blurHandler->update({12.0f, 32.0f});

    tempTarget = new wrap::OMRendererTempTarget(renderer);

    auto ext = renderer->getExtent();
    tempTarget->construct(ext);

    pipeline = renderer->createPipeline()
                   ->input(UniformBuffer)
                   ->inputName("ObjectInfo")
                   ->input(ImageSampler)
                   ->inputName("inTexture")
                   ->output(tempTarget->target)
                   ->shader(objectFrg)
                   ->shader(objectVtx)
                   ->format(objModel->format)
                   ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                   ->blend(true)
                   ->depth(true, true)
                   ->depthReverseZ(true)
                   ->buildN();
    pipeline->bindInput(0, uniformBuffer);
    pipeline->bindInput(1, textureImage);
}

static float ang = 0.0f;

void OMTestRenderer::beforeFrame()
{
    if (!timing)
    {
        tp = std::chrono::high_resolution_clock::now();
        timing = true;
    }
    UniformStructure ubo;
    ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.viewProj = camera->fetchProjMat() * camera->fetchViewMat();
    ubo.lightDirection = glm::normalize(glm::vec3(-std::cos(ang), -std::sin(ang), 0.0));
    ubo.lightColor = glm::vec3(1.0);
    ubo.ambientColor = glm::vec3(0.05);

    uniformBuffer->updateData(&ubo);
    ang += 0.001f;
}

void OMTestRenderer::keyInput(bool w, bool a, bool s, bool d, bool lsh, bool sp)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float>(currentTime - startTime).count();
    startTime = currentTime;

    if (w)
    {
        camera->moveCamera(basics::Forward, m_cameraMoveSpeed * time);
    }
    if (s)
    {
        camera->moveCamera(basics::Back, m_cameraMoveSpeed * time);
    }
    if (a)
    {
        camera->moveCamera(basics::Left, m_cameraMoveSpeed * time);
    }
    if (d)
    {
        camera->moveCamera(basics::Right, m_cameraMoveSpeed * time);
    }
    if (sp)
    {
        camera->moveCamera(basics::Up, m_cameraMoveSpeed * time);
    }
    if (lsh)
    {
        camera->moveCamera(basics::Down, m_cameraMoveSpeed * time);
    }
}

void OMTestRenderer::afterFrame()
{
}

void OMTestRenderer::submitTasks()
{
    tempTarget->construct(renderer->getExtent());

    mainPipeline->bindInput(0, tempTarget->colorTexture);
    blurHandler->bind(overlay(), tempTarget->colorTexture);

    auto scene = renderer->createTask("scene")
                     ->clearColor({0.198f, 0.371f, 1.0f, 1.0f})
                     ->clearDepth(0.0f)
                     ->target(tempTarget->target)
                     ->pipeline(pipeline)
                     ->vertexBuffer({objModel->vertexData})
                     ->indexBuffer(objModel->vertexIndex)
                     ->drawIndexedN(objModel->vertexCount)
                     ->finishN();
    blurHandler
        ->secondLayerTask(renderer->createTask("main")
                              ->dependOn(scene)
                              ->dependOn(blurHandler->firstLayerTask(scene))
                              ->dependOn(renderer->fetchTask("demiurgeui_compose"))
                              ->target(renderer->getDefaultRenderTarget())
                              ->pipeline(mainPipeline)
                              ->drawInstanceN(6, 1))
        ->finishN();
}
OMTestRenderer::~OMTestRenderer()
{
    delete mainPipeline;
    delete pipeline;
    delete textureImage;
    delete uniformBuffer;
    delete objModel;

    delete tempTarget;
}
} // namespace openminecraft::boot::test
