#include "openminecraft/boot/entrypoint_testrenderer.hpp"

#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float2.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/specs/png/om_png.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

#include <chrono>
#include <cmath>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "openminecraft/io/om_io_utils.hpp"
#include "SDL3/SDL_events.h"

using namespace openminecraft::renderer::common;

namespace openminecraft::boot::test
{
static auto approximateCubicToQuadratic(glm::vec2 P0, glm::vec2 P1, glm::vec2 P2, glm::vec2 P3) -> glm::vec2
{
    glm::vec2 d1 = P1 - P0;
    glm::vec2 d2 = P3 - P2;
    float det = d1.x * d2.y - d1.y * d2.x;
    if (fabs(det) < 1e-6)
    {
        return (P1 + P2) * glm::vec2(0.5);
    }
    glm::vec2 diff = P3 - P0;
    float s = (diff.x * d2.y - diff.y * d2.x) / det;

    return P0 + s * d1;
}

OMTestRenderer::OMTestRenderer(renderer::OMRenderer *renderer)
    : renderer(renderer), logger("OMTestRenderer", this), OMRendererHandler(renderer)
{
    camera = std::make_shared<basics::OMCamera>(renderer, m_cameraPos, m_yaw, m_pitch);

#define shaderDef(name, filename, type)                                                                                \
    {                                                                                                                  \
        auto target = vfs::fsfetch(fmt::format("/bootassets/openminecraft-renderer/shaders/{}", filename));            \
        name = std::make_shared<OMShader>(GLSLSource, io::readOnce(target.get()), filename, "main", type);             \
    }
    // INFO: basic shaders for renderer
    shaderDef(objectFrg, "objectbase.frag.glsl", Fragment);
    shaderDef(objectVtx, "objectbase.vert.glsl", Vertex);
    shaderDef(outputFrg, "output.frag.glsl", Fragment);
    shaderDef(outputVtx, "output.vert.glsl", Vertex);

    format.appendPart("position", basics::Vec3f);
    format.appendPart("textureUV", basics::Vec2f);
    format.nextGroup();
    format.decideStruct();
    format.debugState();

    {
        glyphStorage = renderer->allocateBuffer(ShaderStorage, 1024 * sizeof(float));

        auto rawfile = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");

        fontproc::OMFontSet set;
        set.fontList.push_back(std::make_shared<fontproc::OMFont>(*rawfile.get()));

        auto s = set.shape("G");
        // logger.debug("{}", s[0].glyphId);
        auto finalData = set.genOutline(s[0].font, s[0].glyphId);
        glyphStorage->updateDataPart(finalData.data(), 0, sizeof(float) * finalData.size());

        std::vector<VertexStruct> vertices = {
            {{0.0f, 0.0f, 0.0f}, {-0.2f, -0.2f}},
            {{0.0f, 0.1f, 0.0f}, {-0.2f, 1.0f}},
            {{0.1f, 0.1f, 0.0f}, {1.0f, 1.0f}},
            {{0.1f, 0.0f, 0.0f}, {1.0f, -0.2f}},
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

    tempUniformBuffer = renderer->allocateBuffer(Uniform, sizeof(UniformStructure));
    UniformStructure stru = {glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), 1, 3};
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
                       ->input(UniformBuffer)
                       ->input(ImageSampler)
                       ->output(renderer->getDefaultRenderTarget())
                       ->shader(outputFrg)
                       ->shader(outputVtx)
                       ->format(format)
                       ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                       ->blend(true)
                       ->depth(true, true)
                       ->buildN();
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
        pipeline = renderer->createPipeline()
                       ->input(UniformBuffer)
                       ->input(ImageSampler)
                       ->input(ShaderStorageBuffer)
                       ->output(renderTarget)
                       ->shader(objectFrg)
                       ->shader(objectVtx)
                       ->format(format)
                       ->blendFunc({Alpha, OneMinusAlpha, Alpha, OneMinusAlpha})
                       ->blend(true)
                       ->depth(true, true)
                       ->buildN();
        pipeline->bindInput(0, uniformBuffer);
        pipeline->bindInput(1, textureImage);
        pipeline->bindInput(2, glyphStorage);
    }

    mainPipeline->bindInput(1, tempTexture);

    auto task = renderer->createTask()
                    ->target(renderTarget)
                    ->pipeline(pipeline)
                    ->clearN()
                    ->vertexBuffer({vertexBuffer})
                    ->indexBuffer(indexBuffer)
                    ->drawN(vertexCount)
                    ->finishN();

    auto task2 = renderer->createTask()
                     ->dependOn(task)
                     ->target(renderer->getDefaultRenderTarget())
                     ->pipeline(mainPipeline)
                     ->clearN()
                     ->vertexBuffer({mainVtxBuffer})
                     ->indexBuffer(mainIdxBuffer)
                     ->drawN(6)
                     ->finishN();
    renderer->registerTask("main", task2);
    renderer->registerTask("intermediate", task);

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

    delete glyphStorage;

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
    while (SDL_PollEvent(&e))
    {
        switch (e.type)
        {
        case SDL_EVENT_KEY_DOWN:
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
                SDL_SetWindowRelativeMouseMode(reinterpret_cast<SDL_Window *>(wnd), false);
            }
            break;
        case SDL_EVENT_KEY_UP:
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
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            SDL_SetWindowRelativeMouseMode(reinterpret_cast<SDL_Window *>(wnd), true);
            break;
        case SDL_EVENT_MOUSE_MOTION: {
            if (SDL_GetWindowRelativeMouseMode(reinterpret_cast<SDL_Window *>(wnd)))
            {
                int ww, hh;
                SDL_GetWindowSize(reinterpret_cast<SDL_Window *>(wnd), &ww, &hh);
                mouseOffset(e.motion.xrel / ww, e.motion.yrel / hh);
            }
            break;
        }
        case SDL_EVENT_QUIT:
            ar[0] = true;
            return;
        case SDL_EVENT_WINDOW_RESIZED:
            ar[1] = true;
            break;
        }

        keyInput(wk, ak, sk, dk, lshk, spk);
    }
}

} // namespace openminecraft::boot::test
