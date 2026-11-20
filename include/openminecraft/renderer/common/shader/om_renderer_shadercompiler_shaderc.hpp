#ifndef OM_RENDERER_SHADERCOMPILER_SHADERC_HPP
#define OM_RENDERER_SHADERCOMPILER_SHADERC_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_shadercompiler.hpp"
#include "shaderc/shaderc.hpp"
#include <memory>
namespace openminecraft::renderer::common
{
class OMRendererShaderCompilerBackendShaderc : public OMRendererShaderCompilerBackend
{
  public:
    OMRendererShaderCompilerBackendShaderc() : logger("OMRendererShaderCompilerBackendShaderc", this)
    {
        compiler = std::make_shared<shaderc::Compiler>();
    }
    virtual ~OMRendererShaderCompilerBackendShaderc() = default;

    auto compile(std::shared_ptr<OMShader> shader) -> std::shared_ptr<OMShader> override;

    auto outputType() -> OMShaderFileType override
    {
        return common::SPIRVBinary;
    }

    auto accept(OMShaderFileType ftype) -> bool override
    {
        return ftype == common::GLSLSource || ftype == common::HLSLSource;
    }

  private:
    std::shared_ptr<shaderc::Compiler> compiler;
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif
