#ifndef OM_RENDERER_LAYER_OPENGL_TASK_HPP
#define OM_RENDERER_LAYER_OPENGL_TASK_HPP

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
    ClearDepth,
    ClearBufferfv,
    Disable,
    Enable,
    BindVertexArray,
    BindBufferBase,
    ActiveTexture,
    BindTexture,
    UseProgram,
    DrawElements,
    DrawElementsInstanced,
    MultiDrawElementsIndirect,
    BindBuffer,
    BlendFunc,
    DepthFunc,
    BlendFuncSeparate,
    DepthMask,
    DrawElementsInstancedBaseInstance,
    ClearColor,
    DrawArraysInstanced,
    BlitFramebuffer,
    DrawArrays,
    PolygonMode,
    CullFace,
    FrontFace,
    LineWidth,
    PolygonOffset,
    BlendColor,
    BlendEquationSeparate,
    LogicOp,
    Viewport,
    Scissor
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
    std::array<GLuint, 12> args;
    std::array<void *, 2> ptrArgs;
    std::array<GLfloat, 8> floatArgs;
};
class OMRendererTaskOpenGL : public common::OMRendererTask
{
  public:
    OMRendererTaskOpenGL(OMRendererOpenGL *renderer);
    ~OMRendererTaskOpenGL() override;

    void bindPipeline(common::OMRendererPipeline *pipeline) override;
    void bindVertexBuffer(std::vector<common::OMRendererBuffer *> buffer) override;
    void bindVertexBufferInstanced(std::vector<common::OMRendererBuffer *> buffer, int off) override;
    void bindIndexBuffer(common::OMRendererBuffer *buffer) override;
    void bindIndirectBuffer(common::OMRendererBuffer *buffer) override;
    void bindTarget(common::OMRendererRenderTarget *target) override;
    void draw(uint64_t vertexCount) override;
    void drawInstance(uint64_t vertexCount, uint64_t instanceCount) override;
    void drawInstance(uint64_t vertexCount, uint64_t instanceCount, uint64_t firstInstance) override;
    void drawIndexed(uint64_t vertexCount) override;
    void drawIndexedInstance(uint64_t vertexCount, uint64_t instanceCount) override;
    void drawIndexedInstance(uint64_t vertexCount, uint64_t instanceCount, uint64_t firstInstance) override;
    void drawIndirect(uint64_t begin, uint64_t count) override;
    void finish() override;
    void resolveTo(common::OMRendererRenderTarget *target) override;

    void execute();

  private:
    auto primitiveType() -> GLenum;
    OMRendererOpenGLFuncs *gl;
    OMRendererPipelineOpenGL *pipeline;
    std::vector<GLuint> vaos;

    GLuint program;
    GLuint framebuffer;
    common::basics::OMVertexFormat vtxFormat;

    std::vector<OMRendererTaskOp> ops;
    bool isCleared = false;
    bool needClearDepth = false;
};
} // namespace openminecraft::renderer::opengl

#endif
