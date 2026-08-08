#ifndef OM_RENDERER_LAYER_OPENGL_PIPELINE_HPP
#define OM_RENDERER_LAYER_OPENGL_PIPELINE_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
#include "openminecraft/renderer/opengl/om_renderer_layer_opengl.hpp"
#include <memory>
#include <vector>

namespace openminecraft::renderer::opengl
{
class OMRendererPipelineOpenGL : public common::OMRendererPipeline
{
  public:
    OMRendererPipelineOpenGL(OMRendererOpenGL *renderer);
    ~OMRendererPipelineOpenGL() override;

    void appendInput(common::OMRendererPipelineInputType) override;
    void attachShader(std::shared_ptr<common::OMShader> shader) override;
    void vertexFormat(common::basics::OMVertexFormat format) override;
    void bindOutput(common::OMRendererRenderTarget *target) override;
    void build() override;
    void setBlendFunc(common::OMReedererPipelineBlendState state) override;

    void bindInput(int idx, common::OMRendererBuffer *buff) override;
    void bindInput(int idx, common::OMRendererTexture *texture) override;
    void bindInputName(std::string name) override;

    common::OMRendererRenderTarget *target;
    GLuint program = 0;
    common::basics::OMVertexFormat format;
    std::vector<OMRendererObject *> inputs;
    std::vector<common::OMRendererPipelineInputType> inputTypes;
    std::vector<std::string> inputNames;
    common::OMReedererPipelineBlendState blendState;

  private:
    OMRendererOpenGLFuncs *gl;
    std::vector<std::shared_ptr<common::OMShader>> preshaders;
};
} // namespace openminecraft::renderer::opengl

#endif
