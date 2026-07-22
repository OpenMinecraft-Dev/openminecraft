#ifndef OM_RENDERER_LAYER_OPENGL_TASK_HPP
#define OM_RENDERER_LAYER_OPENGL_TASK_HPP

#include "GL/glcorearb.h"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl_pipeline.hpp"
#include <array>
#include <cstdint>
#include <vector>
namespace openminecraft::renderer::opengl
{
static const std::array<std::string, 9> opnames = {"BindFramebuffer", "Clear",          "Disable",
                                                   "BindVertexArray", "BindBufferBase", "ActiveTexture",
                                                   "BindTexture",     "UseProgram",     "DrawElements"};
enum OMRendererOpType
{
    BindFramebuffer,
    Clear,
    Disable,
    Enable,
    BindVertexArray,
    BindBufferBase,
    ActiveTexture,
    BindTexture,
    UseProgram,
    DrawElements,
    DrawElementsInstanced
};
union OMRendererOpenGLArg {
    GLuint i;
    void *p;

    OMRendererOpenGLArg(void *p)
    {
        this->p = p;
    }

    OMRendererOpenGLArg(GLuint i)
    {
        this->i = i;
    }
};
struct OMRendererTaskOp
{
    OMRendererOpType type;
    std::array<GLuint, 8> args;
    std::array<void *, 2> ptrArgs;
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
    void drawInstance(uint64_t vertexCount, uint64_t instanceCount) override;
    void finish() override;
    void clear() override;

    void execute();

  private:
    OMRendererOpenGLFuncs *gl;
    OMRendererPipelineOpenGL *pipeline;
    // GLuint vertexArrayObject;
    std::vector<GLuint> vaos;

    GLuint program;
    GLuint framebuffer;
    common::basics::OMVertexFormat vtxFormat;

    std::vector<OMRendererTaskOp> ops;
};
} // namespace openminecraft::renderer::opengl

#endif
