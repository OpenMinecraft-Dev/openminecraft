#include "openminecraft/renderer/common/shader/om_renderer_shadercompiler_shaderc.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "shaderc/shaderc.h"
#include "shaderc/shaderc.hpp"
#include <fstream>
#include <memory>

namespace openminecraft::renderer::common
{
auto OMRendererShaderCompilerBackendShaderc::compile(std::shared_ptr<OMShader> shader) -> std::shared_ptr<OMShader>
{
    shaderc_shader_kind k = {};
    switch (shader->typebase)
    {
    case Vertex:
        k = shaderc_vertex_shader;
        break;
    case Fragment:
        k = shaderc_fragment_shader;
        break;
    case Geometry:
        k = shaderc_geometry_shader;
        break;
    case Compute:
        k = shaderc_compute_shader;
        break;
    case TessControl:
        k = shaderc_tess_control_shader;
        break;
    case TessEvaluation:
        k = shaderc_tess_evaluation_shader;
        break;
    case RayGen:
        k = shaderc_raygen_shader;
        break;
    case AnyHit:
        k = shaderc_anyhit_shader;
        break;
    case ClosestHit:
        k = shaderc_closesthit_shader;
        break;
    case Miss:
        k = shaderc_miss_shader;
        break;
    case Intersection:
        k = shaderc_intersection_shader;
        break;
    case Callable:
        k = shaderc_callable_shader;
        break;
    default:
        break;
    }

    shaderc::CompileOptions opt;

    opt.SetSourceLanguage(shader->type == GLSLSource ? shaderc_source_language_glsl : shaderc_source_language_hlsl);
    // opt.SetOptimizationLevel(shaderc_optimization_level_performance);
    opt.SetAutoBindUniforms(true);
    opt.SetAutoSampledTextures(true);
    opt.SetPreserveBindings(true);

    auto result = compiler->CompileGlslToSpv(reinterpret_cast<const char *>(shader->data.data()), shader->data.size(),
                                             k, shader->filename.c_str(), shader->entrypoint.c_str(), opt);

    logger.info("complied shader {}", shader->filename);
    logger.info("{} errors, {} warnings", result.GetNumErrors(), result.GetNumWarnings());
    if (result.GetNumWarnings() != 0 && result.GetNumErrors() == 0)
    {
        logger.warn(result.GetErrorMessage());
    }
    if (result.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        throw OMRendererException(
            fmt::format("Shader compliation failed for {}: \n{}", shader->filename, result.GetErrorMessage()));
    }

    std::vector<uint8_t> data;
    for (unsigned int itt : result)
    {
        uint8_t l1 = itt;
        uint8_t l2 = itt >> 8;
        uint8_t l3 = itt >> 16;
        uint8_t l4 = itt >> 24;
        data.push_back(l1);
        data.push_back(l2);
        data.push_back(l3);
        data.push_back(l4);
    }

    return std::make_shared<OMShader>(SPIRVBinary, data, shader->filename + ".spirv", shader->entrypoint,
                                      shader->typebase);
}
} // namespace openminecraft::renderer::common
