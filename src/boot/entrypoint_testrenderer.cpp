#include "openminecraft/boot/entrypoint_testrenderer.hpp"

#include "SDL3/SDL_video.h"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "tiny_obj_loader.h"

#include <chrono>
#include <glm/glm.hpp>
#include <stdexcept>

#include "openminecraft/io/om_io_utils.hpp"
#include "SDL3/SDL_events.h"

using namespace openminecraft::renderer::common;

namespace openminecraft::boot::test
{
OMTestRenderer::OMTestRenderer(renderer::OMRenderer *renderer)
    : renderer(renderer), logger("OMTestRenderer", this), OMRendererHandler(renderer)
{
    camera = std::make_shared<basics::OMCamera>(renderer, m_cameraPos, m_yaw, m_pitch);

#define shaderDef(name, filename, type)                                                                                \
    {                                                                                                                  \
        auto target = vfs::fsfetch(fmt::format("/bootassets/openminecraft-renderer/shaders/{}", filename));            \
        name = std::make_shared<OMShader>(GLSLSource, io::readOnce(target.get()), filename, "main", type);             \
    }

    shaderDef(objectFrg, "objectbase.frag.glsl", Fragment);
    shaderDef(objectVtx, "objectbase.vert.glsl", Vertex);
    shaderDef(outputFrg, "output.frag.glsl", Fragment);
    shaderDef(outputVtx, "output.vert.glsl", Vertex);

    basics::OMVertexFormat format;
    format.appendPart("position", basics::Vec3f);
    format.appendPart("textureUV", basics::Vec2f);
    format.nextGroup();
    format.decideStruct();
    format.debugState();

    {
        class VertexPart
        {
            glm::vec3 pos;
            glm::vec2 textureUV;

          public:
            VertexPart(glm::vec3 p, glm::vec2 uv) : pos(p), textureUV(uv)
            {
            }

            auto operator<(const VertexPart &other) const -> bool
            {
                return std::memcmp(&other, this, sizeof(VertexPart)) < 0;
            }
        };

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        auto strr = vfs::fsfetch("/bootassets/openminecraft-renderer/models/viking_room.obj");

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, strr.get()))
        {
            throw std::runtime_error("Warn: " + warn + "\nError: " + err);
        }

        auto iff = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
        auto f = new fontproc::OMFont(*iff.get());
        auto ppo = f->buildBasicPolygon(0x2299);
        delete f;

        float ratio = 1.377143f;

        std::vector<VertexPart> vtxnew;
        std::vector<uint32_t> indices;

        vtxnew.clear();
        indices.clear();

        for (auto &v : ppo->vertices)
        {
            vtxnew.push_back({{v.x, v.y, 0.0f}, {v.x, v.y}});
        }

        for (auto i : ppo->indices)
        {
            indices.push_back(i);
        }

        /*std::map<VertexPart, uint32_t> uniqueVertices;

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                VertexPart prt = {{
                                      attrib.vertices[3 * index.vertex_index + 0],
                                      attrib.vertices[3 * index.vertex_index + 2],
                                      attrib.vertices[3 * index.vertex_index + 1],
                                  },
                                  {attrib.texcoords[2 * index.texcoord_index + 0],
                                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]}};

                if (!uniqueVertices.count(prt))
                {
                    uniqueVertices[prt] = static_cast<uint32_t>(vtxnew.size());
                    vtxnew.push_back(prt);
                }
                indices.push_back(uniqueVertices[prt]);
            }
        }*/

        auto siz = vtxnew.size() * sizeof(VertexPart);

        vertexBuffer = renderer->allocateBuffer(VertexData, siz);
        vertexBuffer->updateData(vtxnew.data());

        siz = indices.size() * sizeof(uint32_t);
        indexBuffer = renderer->allocateBuffer(VertexIndex, siz);
        indexBuffer->updateData(indices.data());

        vertexCount = indices.size();
    }

    {
        mainVtxBuffer = renderer->allocateBuffer(VertexData, 4 * (3 + 2) * sizeof(float));
        mainIdxBuffer = renderer->allocateBuffer(VertexIndex, 6 * sizeof(uint32_t));

        std::array<float, 20> vtxs = {-1.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f,  0.0f, 0.0f, 1.0f,
                                      1.0f,  1.0f,  0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f};
        std::array<uint32_t, 6> vtxi = {0, 1, 2, 2, 3, 0};
        mainVtxBuffer->updateData(vtxs.data());
        mainIdxBuffer->updateData(vtxi.data());
    }

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(UniformStructure));

    tempUniformBuffer = renderer->allocateBuffer(Uniform, sizeof(UniformStructure));
    UniformStructure stru = {};
    stru.model = glm::mat4(1.0f);
    stru.proj = glm::mat4(1.0f);
    stru.view = glm::mat4(1.0f);
    stru.kernelSize = 9;
    stru.sigma = 3;
    tempUniformBuffer->updateData(&stru);

    {
        auto imgraw = vfs::fsfetch("/bootassets/openminecraft-renderer/texture/viking_room.png");

        specs::png::OMPngFile img;
        img.parse(imgraw);

        textureImage = renderer->allocateTexture(img.getWidth(), img.getHeight(), Dim2, ColorRgba);
        textureImage->updateData(img.fetchData());
    }

    mainPipeline = renderer->createPipeline();
    mainPipeline->appendInput(UniformBuffer);
    mainPipeline->appendInput(ImageSampler);
    mainPipeline->bindOutput(renderer->getDefaultRenderTarget());
    mainPipeline->attachShader(outputFrg);
    mainPipeline->attachShader(outputVtx);
    mainPipeline->vertexFormat(format);
    mainPipeline->build();
    mainPipeline->bindInput(0, tempUniformBuffer);
} // namespace openminecraft::boot::test

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
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - tp).count();
    fps++;
    if (duration > 500.0 * 1000 * 1000)
    {
        logger.info("{:.2f} fps (average ~500ms)", fps * 1000.0 * 1000 * 1000 / static_cast<double>(duration));
        timing = false;
        fps = 0;
    }
}

void OMTestRenderer::submitTasks()
{
    if (!firstTime)
    {
        delete tempTexture;
        delete tempDepth;
    }

    auto ext = renderer->getExtent();
    tempTexture = renderer->allocateTexture(ext.x, ext.y, Dim2, ColorRgba);
    tempDepth = renderer->allocateTexture(ext.x, ext.y, Dim2, Depth);

    if (firstTime)
    {
        renderTarget = renderer->createRenderTarget();
        renderTarget->attachTarget(tempTexture);
        renderTarget->attachTarget(tempDepth);
        renderTarget->build();
    }
    else
    {
        renderTarget->replaceTarget(0, tempTexture);
        renderTarget->replaceTarget(1, tempDepth);
        renderTarget->rebuild();
    }

    if (firstTime)
    {
        pipeline = renderer->createPipeline();
        pipeline->appendInput(UniformBuffer);
        pipeline->appendInput(ImageSampler);
        pipeline->bindOutput(renderTarget);
        pipeline->attachShader(objectFrg);
        pipeline->attachShader(objectVtx);
        basics::OMVertexFormat format;
        format.appendPart("position", basics::Vec3f);
        format.appendPart("textureUV", basics::Vec2f);
        format.nextGroup();
        format.decideStruct();
        format.debugState();
        pipeline->vertexFormat(format);
        pipeline->build();
        pipeline->bindInput(0, uniformBuffer);
        pipeline->bindInput(1, textureImage);
    }

    mainPipeline->bindInput(1, tempTexture);

    auto task = renderer->createTask();
    task->bindTarget(renderTarget);
    task->bindPipeline(pipeline);
    task->bindVertexBuffer({vertexBuffer});
    task->bindIndexBuffer(indexBuffer);
    task->draw(vertexCount);
    task->finish();

    auto task2 = renderer->createTask();
    task2->bindTarget(renderer->getDefaultRenderTarget());
    task2->bindPipeline(mainPipeline);
    task2->bindVertexBuffer({mainVtxBuffer});
    task2->bindIndexBuffer(mainIdxBuffer);
    task2->draw(6);
    task2->finish();

    renderer->registerTask("main", task2);
    renderer->registerTask("intermediate", task);

    logger.info("Task intermediate: {}", fmt::ptr(renderer->fetchTask("intermediate")));
    logger.info("Task main: {}", fmt::ptr(renderer->fetchTask("main")));

    firstTime = false;
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

    delete tempTexture;
    delete tempDepth;
    delete renderTarget;
}
static bool wk = false, ak = false, sk = false, dk = false, spk = false, lshk = false;

void OMTestRenderer::eventLoop(void *wnd, bool *ar)
{
    SDL_Event e;
    SDL_PollEvent(&e);

    if (e.type == SDL_EVENT_KEY_DOWN)
    {
        if (e.key.key == SDLK_W)
        {
            wk = true;
        }
        else if (e.key.key == SDLK_A)
        {
            ak = true;
        }
        else if (e.key.key == SDLK_S)
        {
            sk = true;
        }
        else if (e.key.key == SDLK_D)
        {
            dk = true;
        }
        else if (e.key.key == SDLK_LSHIFT)
        {
            lshk = true;
        }
        else if (e.key.key == SDLK_SPACE)
        {
            spk = true;
        }
        else if (e.key.key == SDLK_ESCAPE)
        {
            SDL_SetWindowRelativeMouseMode((SDL_Window *)wnd, false);
        }
    }

    if (e.type == SDL_EVENT_KEY_UP)
    {
        if (e.key.key == SDLK_W)
        {
            wk = false;
        }
        else if (e.key.key == SDLK_A)
        {
            ak = false;
        }
        else if (e.key.key == SDLK_S)
        {
            sk = false;
        }
        else if (e.key.key == SDLK_D)
        {
            dk = false;
        }
        else if (e.key.key == SDLK_LSHIFT)
        {
            lshk = false;
        }
        else if (e.key.key == SDLK_SPACE)
        {
            spk = false;
        }
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        SDL_SetWindowRelativeMouseMode((SDL_Window *)wnd, true);
    }

    if (e.type == SDL_EVENT_MOUSE_MOTION && SDL_GetWindowRelativeMouseMode((SDL_Window *)wnd))
    {
        int ww, hh;
        SDL_GetWindowSize((SDL_Window *)wnd, &ww, &hh);
        mouseOffset(e.motion.xrel / ww, e.motion.yrel / hh);
    }

    if (e.type == SDL_EVENT_QUIT)
    {
        ar[0] = true;
    }

    if (e.type == SDL_EVENT_WINDOW_RESIZED)
    {
        ar[1] = true;
    }

    keyInput(wk, ak, sk, dk, lshk, spk);
}

} // namespace openminecraft::boot::test
