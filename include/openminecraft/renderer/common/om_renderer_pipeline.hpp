#ifndef OM_RENDERER_PIPELINE_HPP
#define OM_RENDERER_PIPELINE_HPP

#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
#include <memory>
namespace openminecraft::renderer
{
class OMRenderer;
}

namespace openminecraft::renderer::common
{
class OMShader;
class OMRendererRenderTarget;
enum OMRendererPipelineInputType
{
    ImageSampler,
    UniformBuffer,
    ShaderStorageBuffer
};
enum OMRendererPipelineBlendType
{
    One,
    Zero,
    Alpha,
    OneMinusAlpha
};
struct OMReedererPipelineBlendState
{
    OMRendererPipelineBlendType srcColor, dstColor, srcAlpha, dstAlpha;
};
class OMRendererPipeline : public OMRendererObject
{
  public:
    OMRendererPipeline(OMRenderer *renderer)
    {
    }
    virtual ~OMRendererPipeline() = default;

    virtual void appendInput(OMRendererPipelineInputType) = 0;
    inline auto input(OMRendererPipelineInputType in) -> OMRendererPipeline *
    {
        appendInput(in);
        return this;
    }
    virtual void attachShader(std::shared_ptr<OMShader> shader) = 0;
    inline auto shader(std::shared_ptr<OMShader> shader) -> OMRendererPipeline *
    {
        attachShader(shader);
        return this;
    }
    virtual void vertexFormat(basics::OMVertexFormat format) = 0;
    inline auto format(basics::OMVertexFormat format) -> OMRendererPipeline *
    {
        vertexFormat(format);
        return this;
    }
    virtual void bindOutput(OMRendererRenderTarget *target) = 0;
    inline auto output(OMRendererRenderTarget *target) -> OMRendererPipeline *
    {
        bindOutput(target);
        return this;
    }
    virtual void build() = 0;
    inline auto buildN() -> OMRendererPipeline *
    {
        build();
        return this;
    }

    virtual void setBlendFunc(OMReedererPipelineBlendState state) = 0;
    inline auto blendFunc(OMReedererPipelineBlendState state) -> OMRendererPipeline *
    {
        setBlendFunc(state);
        return this;
    }

    inline auto blend(bool enable) -> OMRendererPipeline *
    {
        enableBlend = enable;
        return this;
    }
    inline auto depth(bool test, bool write) -> OMRendererPipeline *
    {
        enableDepthTest = test;
        enableDepthWrite = write;
        return this;
    }
    inline auto depthReverseZ(bool reverse) -> OMRendererPipeline *
    {
        enableReverseZ = reverse;
        return this;
    }

    virtual void bindInput(int idx, common::OMRendererBuffer *buff) = 0;
    virtual void bindInput(int idx, common::OMRendererTexture *texture) = 0;
    virtual void bindInputName(int idx, std::string name) = 0;
    inline auto inputName(int idx, std::string name) -> OMRendererPipeline *
    {
        bindInputName(idx, name);
        return this;
    }

    inline auto objType() -> OMRendererObjectType override
    {
        return Pipeline;
    }

    bool enableBlend = false;
    bool enableDepthTest = false;
    bool enableDepthWrite = false;
    bool enableReverseZ = false;
};
} // namespace openminecraft::renderer::common

#endif
