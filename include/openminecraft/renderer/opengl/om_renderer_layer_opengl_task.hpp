#ifndef OM_RENDERER_LAYER_OPENGL_TASK_HPP
#define OM_RENDERER_LAYER_OPENGL_TASK_HPP

#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include <array>
namespace openminecraft::renderer::opengl
{
enum OMRendererOpType
{
    BindFramebuffer,
    Clear,
    Disable,
    BindVertexArray,
    BindBufferBase,
    ActiveTexture,
    BindTexture,
    UseProgram
};
struct OMRendererTaskOp
{
    OMRendererOpType type;
    std::array<GLuint, 8> args;
};
class OMRendererTaskOpenGL : public common::OMRendererTask
{
  public:
    OMRendererTaskOpenGL(OMRendererOpenGL *renderer);
    ~OMRendererTaskOpenGL() override;

    void bindPipeline(common::OMRendererPipeline *pipeline) override;
    void bindVertexBuffer(std::vector<common::OMRendererBuffer *> buffer) override;
    void bindIndexBuffer(common::OMRendererBuffer *buffer) override;
    void bindTarget(common::OMRendererRenderTarget *target) override;
    void draw(uint64_t vertexCount) override;
    void finish() override;

    void execute();

  private:
    OMRendererOpenGLFuncs *gl;
    OMRendererPipelineOpenGL *pipeline;
    GLuint vertexArrayObject;

    GLuint program;
    GLuint framebuffer;
    common::basics::OMVertexFormat vtxFormat;
    uint64_t vtxCount;
};
} // namespace openminecraft::renderer::opengl

#endif
