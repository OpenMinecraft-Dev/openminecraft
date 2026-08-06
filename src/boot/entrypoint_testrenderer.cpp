#include "openminecraft/boot/entrypoint_testrenderer.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_temptarget.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

#include <chrono>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

using namespace openminecraft::renderer::common;

namespace openminecraft::boot::test
{
OMTestRenderer::OMTestRenderer(renderer::OMRenderer *renderer, std::function<OMRendererTexture *()> overlay,
                               event::OMEventBusSDL &bus)
    : renderer(renderer), logger("OMTestRenderer", this), OMRendererHandler(renderer)
{
    this->overlay = overlay;
    camera = std::make_shared<basics::OMCamera>(renderer, glm::vec3{2.0f, 2.0f, 2.0f}, -135.0f, -35.0f);

    // INFO: basic shaders for renderer
    objectFrg = renderer->shaderManager.preprocess("core/objectbase.frag.glsl", Fragment, GLSLSource);
    objectVtx = renderer->shaderManager.preprocess("core/objectbase.vert.glsl", Vertex, GLSLSource);
    outputFrg = renderer->shaderManager.preprocess("core/bilt.frag.glsl", Fragment, GLSLSource);
    outputVtx = renderer->shaderManager.preprocess("core/bilt.vert.glsl", Vertex, GLSLSource);
    outputFrg2 = renderer->shaderManager.preprocess("core/effects/boxblurp2.frag.glsl", Fragment, GLSLSource);
    outputVtx2 = renderer->shaderManager.preprocess("core/effects/boxblurp2.vert.glsl", Vertex, GLSLSource);
    outputFrg3 = renderer->shaderManager.preprocess("core/effects/boxblurp1.frag.glsl", Fragment, GLSLSource);
    outputVtx3 = renderer->shaderManager.preprocess("core/effects/boxblurp1.vert.glsl", Vertex, GLSLSource);

    format.appendPart("position", basics::Vec3f)
        ->appendPart("textureUV", basics::Vec2f)
        ->nextGroup()
        ->decideStruct()
        ->debugState();

    {
        std::vector<VertexStruct> vertices = {
            {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        };
        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};

        auto siz = vertices.size() * sizeof(VertexStruct);

        vertexBuffer = renderer->allocateBuffer(VertexData, siz);
        vertexBuffer->updateData(vertices.data());

        siz = indices.size() * sizeof(uint32_t);
        indexBuffer = renderer->allocateBuffer(VertexIndex, siz);
        indexBuffer->updateData(indices.data());

        vertexCount = indices.size();

        mainVtxBuffer = renderer->allocateBuffer(VertexData, 4 * sizeof(VertexStruct));
        mainIdxBuffer = renderer->allocateBuffer(VertexIndex, 6 * sizeof(uint32_t));

        // INFO: 4 vertices and 6 vertex indices to render a texture to the screen
        std::array<VertexStruct, 4> vtxs = {{
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
            {{-1.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
            {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            {{1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        }};
        std::array<uint32_t, 6> vtxi = {0, 1, 2, 2, 3, 0};
        mainVtxBuffer->updateData(vtxs.data());
        mainIdxBuffer->updateData(vtxi.data());
    }

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(UniformStructure));

    std::array<float, 2> b = {12.0f, 32.0f};
    tempUniformBuffer = renderer->allocateBuffer(Uniform, sizeof(UniformStructure));
    UniformStructure stru = {glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), b[0], b[1]};
    tempUniformBuffer->updateData(&stru);
    blurArgs = renderer->allocateBuffer(Uniform, sizeof(float) * 2);
    blurArgs->updateData(b.data());

    {
        auto imgraw = vfs::fsfetch("/bootassets/openminecraft-renderer/texture/viking_room.png");

        specs::png::OMPngFile img;
        img.parse(imgraw);

        textureImage = renderer->allocateTexture(img.getWidth(), img.getHeight(), Dim2, ColorRgba);
        textureImage->updateData(img.fetchData());
    }

    // INFO: core pipeline creation

    mainPipeline = renderer->createPipeline()
                       ->input(UniformBuffer)
                       ->input(ImageSampler)
                       ->output(renderer->getDefaultRenderTarget())
                       ->shader(outputFrg)
                       ->shader(outputVtx)
                       ->format(format)
                       ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                       ->blend(true)
                       ->depth(false, false)
                       ->buildN();
    mainPipeline->bindInput(0, tempUniformBuffer);
    mainPipeline2 = renderer->createPipeline()
                        ->input(UniformBuffer)
                        ->input(ImageSampler)
                        ->input(ImageSampler)
                        ->output(renderer->getDefaultRenderTarget())
                        ->shader(outputFrg2)
                        ->shader(outputVtx2)
                        ->format(format)
                        ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                        ->blend(true)
                        ->depth(false, false)
                        ->buildN();
    mainPipeline2->bindInput(0, tempUniformBuffer);

    tempTarget = new wrap::OMRendererTempTarget(renderer);
    blurTemp = new wrap::OMRendererTempTarget(renderer);
}

void OMTestRenderer::beforeFrame()
{
    if (!timing)
    {
        tp = std::chrono::high_resolution_clock::now();
        timing = true;
    }
    UniformStructure ubo;
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera->fetchViewMat();
    ubo.proj = camera->fetchProjMat();

    uniformBuffer->updateData(&ubo);
}

void OMTestRenderer::mouseOffset(float dx, float dy)
{
    camera->modPitch(-dy * m_cameraRotateSpeed);
    camera->modYaw(dx * m_cameraRotateSpeed);
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
    auto ext = renderer->getExtent();
    tempTarget->construct(ext);
    blurTemp->construct(ext);

    if (!pipeline)
    {
        pipeline = renderer->createPipeline()
                       ->input(UniformBuffer)
                       ->input(ImageSampler)
                       ->output(tempTarget->target)
                       ->shader(objectFrg)
                       ->shader(objectVtx)
                       ->format(format)
                       ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                       ->blend(true)
                       ->depth(true, true)
                       ->buildN();
        pipeline->bindInput(0, uniformBuffer);
        pipeline->bindInput(1, textureImage);
    }

    if (!mainPipeline3)
    {
        mainPipeline3 = renderer->createPipeline()
                            ->input(UniformBuffer)
                            ->input(ImageSampler)
                            ->output(blurTemp->target)
                            ->shader(outputFrg3)
                            ->shader(outputVtx3)
                            ->format(format)
                            ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                            ->blend(true)
                            ->depth(false, false)
                            ->buildN();
        mainPipeline3->bindInput(0, blurArgs);
    }

    mainPipeline->bindInput(1, tempTarget->colorTexture);
    mainPipeline2->bindInput(1, overlay());
    mainPipeline2->bindInput(2, blurTemp->colorTexture);
    mainPipeline3->bindInput(1, tempTarget->colorTexture);

    auto pretask = renderer->createTask()
                       ->target(tempTarget->target)
                       ->pipeline(pipeline)
                       ->vertexBuffer({vertexBuffer})
                       ->indexBuffer(indexBuffer)
                       ->drawN(vertexCount)
                       ->finishN();
    auto taskimm = renderer->createTask()
                       ->dependOn(pretask)
                       ->target(blurTemp->target)
                       ->pipeline(mainPipeline3)
                       ->vertexBuffer({mainVtxBuffer})
                       ->indexBuffer(mainIdxBuffer)
                       ->drawN(6)
                       ->finishN();
    auto task = renderer->createTask()
                    ->dependOn(pretask)
                    ->dependOn(taskimm)
                    ->dependOn(renderer->fetchTask("demiurgeui_compose"))
                    ->target(renderer->getDefaultRenderTarget())
                    ->pipeline(mainPipeline)
                    ->vertexBuffer({mainVtxBuffer})
                    ->indexBuffer(mainIdxBuffer)
                    ->drawN(6)
                    ->pipeline(mainPipeline2)
                    ->vertexBuffer({mainVtxBuffer})
                    ->indexBuffer(mainIdxBuffer)
                    ->drawN(6)
                    ->finishN();
    renderer->registerTask("main", task);
    renderer->registerTask("pretask", pretask);
    renderer->registerTask("middletask", taskimm);
}
OMTestRenderer::~OMTestRenderer()
{
    delete mainPipeline;
    delete mainPipeline2;
    delete mainPipeline3;
    delete pipeline;
    delete textureImage;
    delete uniformBuffer;
    delete tempUniformBuffer;
    delete blurArgs;
    delete vertexBuffer;
    delete indexBuffer;

    delete mainIdxBuffer;
    delete mainVtxBuffer;

    delete tempTarget;
    delete blurTemp;
}
} // namespace openminecraft::boot::test
