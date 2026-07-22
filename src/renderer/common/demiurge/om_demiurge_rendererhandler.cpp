#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
#include "glm/glm.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <memory>
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "openminecraft/io/om_io_utils.hpp"

namespace openminecraft::renderer::common::demiurge
{
struct SimpleUniform
{
    float width;
    float height;
};
OMDemiurgeRendererHandler::OMDemiurgeRendererHandler(OMRenderer *renderer)
    : renderer(renderer), OMRendererHandler(renderer)
{
    node = std::make_shared<element::OMDemiurgeRectElement>()
               ->flexWrap(OMDemiurgeWrap::Wrap)
               ->flexDirection(OMDemiurgeDirection::Row)
               ->setStyle("color", 0x2c2c34ff);
    for (int i = 0; i < 4; ++i)
    {
        auto d = std::make_shared<element::OMDemiurgeRectElement>()
                     ->width(OMDemiurgeSize::percent(0.45))
                     ->height(OMDemiurgeSize::percent(0.45))
                     ->margin(OMDemiurgeSize::percent(0.025), OMDemiurgeSize::percent(0.025),
                              OMDemiurgeSize::percent(0.025), OMDemiurgeSize::percent(0.025))
                     ->setStyle("color", 0x00d4ffff);
        node->mount(d);
    }

#define shaderDef(name, filename, type)                                                                                \
    {                                                                                                                  \
        auto target = vfs::fsfetch(fmt::format("/bootassets/openminecraft-renderer/shaders/{}", filename));            \
        name = std::make_shared<OMShader>(GLSLSource, io::readOnce(target.get()), filename, "main", type);             \
    }
    shaderDef(vtxShader, "demiurge/rect.vert.glsl", Vertex);
    shaderDef(frgShader, "demiurge/rect.frag.glsl", Fragment);

    format.appendPart("position", basics::Vec3f);
    format.nextGroup();
    format.setInstance();
    format.appendPart("rect_color", basics::Vec4f);
    format.nextGroup();
    format.decideStruct();
    format.debugState();

    uniformBuffer = renderer->allocateBuffer(Uniform, sizeof(SimpleUniform));

    uiPipeline = renderer->createPipeline()
                     ->input(UniformBuffer)
                     ->output(renderer->getDefaultRenderTarget())
                     ->shader(frgShader)
                     ->shader(vtxShader)
                     ->format(format)
                     ->buildN();
    uiPipeline->bindInput(0, uniformBuffer);
}
OMDemiurgeRendererHandler::~OMDemiurgeRendererHandler()
{
    delete uniformBuffer;
    delete uiPipeline;
}

void OMDemiurgeRendererHandler::submitTasks()
{
    auto ext = renderer->getExtent();
    node->layout(ext.x, ext.y);
    SimpleUniform u{ext.x, ext.y};
    uniformBuffer->updateData(&u);

    auto task = renderer->createTask()
                    ->dependOn(renderer->fetchTask("main"))
                    ->target(renderer->getDefaultRenderTarget())
                    ->clearN();
    node->render(task, this, 0.9f);
    task->finishN();
    renderer->registerTask("demiurgeui_test", task);
}
void OMDemiurgeRendererHandler::beforeFrame()
{
}
void OMDemiurgeRendererHandler::afterFrame()
{
}
} // namespace openminecraft::renderer::common::demiurge
