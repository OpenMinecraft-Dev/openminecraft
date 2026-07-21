#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "glm/glm.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <memory>
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/om_io_utils.hpp"

namespace openminecraft::renderer::common::demiurge
{
struct ColoredVertex
{
    glm::vec3 pos;
    glm::vec4 color;
};
OMDemiurgeRendererHandler::OMDemiurgeRendererHandler(OMRenderer *renderer)
    : renderer(renderer), OMRendererHandler(renderer)
{
    node = std::make_shared<OMDemiurgeNode>();

#define shaderDef(name, filename, type)                                                                                \
    {                                                                                                                  \
        auto target = vfs::fsfetch(fmt::format("/bootassets/openminecraft-renderer/shaders/{}", filename));            \
        name = std::make_shared<OMShader>(GLSLSource, io::readOnce(target.get()), filename, "main", type);             \
    }
    shaderDef(vtxShader, "plainbase.vert.glsl", Vertex);
    shaderDef(frgShader, "plainbase.frag.glsl", Fragment);

    format.appendPart("position", basics::Vec3f);
    format.appendPart("color", basics::Vec4f);
    format.nextGroup();
    format.decideStruct();
    format.debugState();

    mainVtxBuffer = renderer->allocateBuffer(VertexData, 2 * 4 * sizeof(ColoredVertex));
    mainIdxBuffer = renderer->allocateBuffer(VertexIndex, 2 * 6 * sizeof(uint32_t));

    // INFO: 4 vertices and 6 vertex indices to render a texture to the screen
    glm::vec4 colr1 = {glm::pow({0.17254901960784313, 0.17254901960784313, 0.20392156862745098}, glm::vec3(2.2)), 1};
    glm::vec4 colr2 = {glm::pow({0, 0.8313725490196079, 1}, glm::vec3(2.2)), 1};
    std::array<ColoredVertex, 8> vtxs = {{
        {{-1.0f, -1.0f, 0.0f}, colr1},
        {{-1.0f, 1.0f, 0.0f}, colr1},
        {{0.0f, 1.0f, 0.0f}, colr1},
        {{0.0f, -1.0f, 0.0f}, colr1},
        {{1.0f, -1.0f, 0.0f}, colr2},
        {{1.0f, 1.0f, 0.0f}, colr2},
        {{0.0f, 1.0f, 0.0f}, colr2},
        {{0.0f, -1.0f, 0.0f}, colr2},
    }};
    std::array<uint32_t, 12> vtxi = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};
    mainVtxBuffer->updateData(vtxs.data());
    mainIdxBuffer->updateData(vtxi.data());

    uiPipeline = renderer->createPipeline()
                     ->output(renderer->getDefaultRenderTarget())
                     ->shader(frgShader)
                     ->shader(vtxShader)
                     ->format(format)
                     ->buildN();
}

OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    delete uiPipeline;
    delete mainVtxBuffer;
    delete mainIdxBuffer;
}

void OMDemiurgeRendererHandler::submitTasks()
{
    auto task = renderer->createTask()
                    ->dependOn(renderer->fetchTask("main"))
                    ->target(renderer->getDefaultRenderTarget())
                    ->clearN()
                    ->pipeline(uiPipeline)
                    ->vertexBuffer({mainVtxBuffer})
                    ->indexBuffer(mainIdxBuffer)
                    ->drawN(12)
                    ->finishN();
    renderer->registerTask("demiurgeui_test", task);
}
void OMDemiurgeRendererHandler::beforeFrame()
{
}
void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
