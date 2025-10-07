#include "openminecraft/renderer/common/om_renderer_shader.hpp"

#include "openminecraft/log/om_log_common.hpp"
#include "shaderc/shaderc.hpp"

namespace openminecraft::renderer::common
{
log::OMLogger logger("SPIRV Compiler");

std::shared_ptr<OMShader> OMShader::convertTo(OMShaderFileType type)
{
    auto compiler = std::make_unique<shaderc::Compiler>();
    if (this->type == type || type == GLSLSource || type == HLSLSource || this->type == SPIRVBinary)
    {
        return nullptr;
    }

    shaderc_shader_kind k = {};
    switch (this->typebase)
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

    opt.SetSourceLanguage(this->type == GLSLSource ? shaderc_source_language_glsl : shaderc_source_language_hlsl);
    auto result = compiler->CompileGlslToSpv(reinterpret_cast<const char *>(this->data.data()), this->data.size(), k, filename.c_str(), entrypoint.c_str(), opt);

    if (result.GetCompilationStatus() == shaderc_compilation_status_success)
    {
        logger.info("complied shader {}", filename);
    }
    else
    {
        logger.error("{}: {}", filename, result.GetErrorMessage());
    }
    logger.info("{} errors, {} warnings", result.GetNumErrors(), result.GetNumWarnings());

    std::vector<uint8_t> data;
    for (auto itt = result.begin(); itt != result.end(); ++itt)
    {
        uint8_t l1 = *itt;
        uint8_t l2 = *itt >> 8;
        uint8_t l3 = *itt >> 16;
        uint8_t l4 = *itt >> 24;
        data.push_back(l1);
        data.push_back(l2);
        data.push_back(l3);
        data.push_back(l4);
    }

    return std::make_shared<OMShader>(SPIRVBinary, data, filename + ".spirv", entrypoint, this->typebase);
}
}