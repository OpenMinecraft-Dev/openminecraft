#include "openminecraft/boot/entrypoint_testrenderer.hpp"

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/geometric.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/event/om_eventbus.hpp"
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

    format.appendPart("position", basics::Vec3f)
        ->appendPart("textureUV", basics::Vec2f)
        ->appendPart("normal", basics::Vec3f)
        ->nextGroup()
        ->decideStruct()
        ->debugState();

    {
        std::vector<VertexStruct> vertices = {};
        std::vector<uint32_t> indices = {};

        {
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;

            bool success =
                tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                                 vfs::fsfetch("/bootassets/openminecraft-renderer/models/viking_room.obj").get());

            std::unordered_map<VertexStruct, uint32_t, VertexHash> uniqueVertices;

            for (const auto &shape : shapes)
            {
                for (const auto &index : shape.mesh.indices)
                {
                    VertexStruct v;

                    v.pos.x = attrib.vertices[3 * index.vertex_index + 0];
                    v.pos.y = attrib.vertices[3 * index.vertex_index + 1];
                    v.pos.z = attrib.vertices[3 * index.vertex_index + 2];

                    if (index.texcoord_index >= 0)
                    {
                        v.uv.x = attrib.texcoords[2 * index.texcoord_index + 0];
                        v.uv.y = attrib.texcoords[2 * index.texcoord_index + 1];
                    }
                    else
                    {
                        v.uv.x = 0.0f;
                        v.uv.y = 0.0f;
                    }

                    v.normal.x = attrib.normals[3 * index.normal_index + 0];
                    v.normal.y = attrib.normals[3 * index.normal_index + 1];
                    v.normal.z = attrib.normals[3 * index.normal_index + 2];

                    auto it = uniqueVertices.find(v);
                    if (it != uniqueVertices.end())
                    {
                        indices.push_back(it->second);
                    }
                    else
                    {
                        auto newIndex = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(v);
                        uniqueVertices[v] = newIndex;
                        indices.push_back(newIndex);
                    }
                }
            }
        }

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

    tempUniformBuffer = renderer->allocateBuffer(Uniform, sizeof(UniformStructure));
    UniformStructure stru = {glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f)};
    tempUniformBuffer->updateData(&stru);

    {
        auto imgraw = vfs::fsfetch("/bootassets/openminecraft-renderer/texture/viking_room.png");

        specs::png::OMPngFile img;
        img.parse(imgraw);

        textureImage = renderer->allocateTexture(img.getWidth(), img.getHeight(), Dim2, ColorRgba);
        textureImage->updateData(img.fetchData());
    }

    // INFO: core pipeline creation
    mainPipeline = renderer->createPipeline()
                       ->input(ImageSampler)
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
    ubo.view = camera->fetchViewMat();
    ubo.proj = camera->fetchProjMat();
    ubo.lightDirection = glm::normalize(glm::vec3(-std::cos(ang), -std::sin(ang), 0.0));
    ubo.lightColor = glm::vec3(1.0);
    ubo.ambientColor = glm::vec3(0.05);

    uniformBuffer->updateData(&ubo);
    ang += 0.001f;
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
    tempTarget->construct(renderer->getExtent());

    mainPipeline->bindInput(0, tempTarget->colorTexture);
    blurHandler->bind(overlay(), tempTarget->colorTexture);

    auto pretask = renderer->createTask()
                       ->target(tempTarget->target)
                       ->pipeline(pipeline)
                       ->vertexBuffer({vertexBuffer})
                       ->indexBuffer(indexBuffer)
                       ->drawN(vertexCount)
                       ->finishN();
    auto taskimm = blurHandler->firstLayerTask(pretask);
    auto task = blurHandler
                    ->secondLayerTask(renderer->createTask()
                                          ->dependOn(pretask)
                                          ->dependOn(taskimm)
                                          ->dependOn(renderer->fetchTask("demiurgeui_compose"))
                                          ->target(renderer->getDefaultRenderTarget())
                                          ->pipeline(mainPipeline)
                                          ->vertexBuffer({mainVtxBuffer})
                                          ->indexBuffer(mainIdxBuffer)
                                          ->drawN(6))
                    ->finishN();
    renderer->registerTask("main", task);
    renderer->registerTask("pretask", pretask);
    renderer->registerTask("middletask", taskimm);
}
OMTestRenderer::~OMTestRenderer()
{
    delete mainPipeline;
    delete pipeline;
    delete textureImage;
    delete uniformBuffer;
    delete tempUniformBuffer;
    delete vertexBuffer;
    delete indexBuffer;

    delete mainIdxBuffer;
    delete mainVtxBuffer;

    delete tempTarget;
}
} // namespace openminecraft::boot::test
