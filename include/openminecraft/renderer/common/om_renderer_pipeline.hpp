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
    ShaderStorageBuffer,
    UniformTexelBuffer
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
enum OMRendererPipelinePrimitive
{
    TriangleList,
    TriangleStrip,
    TriangleFan,
    LineList,
    LineStrip,
    PointList
};

enum OMRendererPipelinePolygonMode
{
    Fill,
    Line,
    Point
};

enum OMRendererPipelineCullMode
{
    None,
    Front,
    Back,
    FrontAndBack
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
    virtual void bindInputName(std::string name) = 0;
    inline auto inputName(std::string name) -> OMRendererPipeline *
    {
        bindInputName(name);
        return this;
    }

    inline auto samples(uint64_t n) -> OMRendererPipeline *
    {
        sampleCount = n;
        return this;
    }

    inline auto primitiveType(OMRendererPipelinePrimitive p) -> OMRendererPipeline *
    {
        primitive = p;
        return this;
    }

    inline auto enableDepthClamp(bool p) -> OMRendererPipeline *
    {
        depthClamp = p;
        return this;
    }

    inline auto setPolygonMode(OMRendererPipelinePolygonMode m) -> OMRendererPipeline *
    {
        polygonMode = m;
        return this;
    }

    inline auto setCullMode(OMRendererPipelineCullMode m) -> OMRendererPipeline *
    {
        cullMode = m;
        return this;
    }

    inline auto setFrontClockwise(bool b) -> OMRendererPipeline *
    {
        cullClockwise = b;
        return this;
    }

    inline auto setLineWidth(float l) -> OMRendererPipeline *
    {
        lineWidth = l;
        return this;
    }

    inline auto depthBias(bool e, float constant, float slope) -> OMRendererPipeline *
    {
        enableDepthBias = e;
        depthBiasConstant = constant;
        depthBiasSlope = slope;
        return this;
    }

    inline auto objType() -> OMRendererObjectType override
    {
        return Pipeline;
    }

    inline auto depthEquals(bool e) -> OMRendererPipeline *
    {
        enableDepthEqual = e;
        return this;
    }

    bool enableBlend = false;
    bool enableDepthTest = false;
    bool enableDepthWrite = false;
    bool enableReverseZ = false;
    uint64_t sampleCount = 1;
    OMRendererPipelinePrimitive primitive = TriangleList;
    bool depthClamp = false;
    OMRendererPipelinePolygonMode polygonMode = Fill;
    OMRendererPipelineCullMode cullMode = None;
    bool cullClockwise = false;
    float lineWidth = 1.0f;
    bool enableDepthBias = true;
    float depthBiasConstant = 0.0f;
    float depthBiasSlope = 0.0f;
    bool enableDepthEqual = false;
};
} // namespace openminecraft::renderer::common

#endif
