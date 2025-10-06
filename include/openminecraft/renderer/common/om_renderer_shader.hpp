#ifndef OM_RENDERER_SHADER_HPP
#define OM_RENDERER_SHADER_HPP
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace openminecraft::renderer::common
{
enum OMShaderFileType
{
    GLSLSource,
    HLSLSource,
    SPIRVBinary
};

enum OMShaderType
{
    Vertex,
    Fragment,
    Geometry,
    Compute,
    TessControl,
    TessEvaluation,
    RayGen,
    AnyHit,
    ClosestHit,
    Miss,
    Intersection,
    Callable
};

class OMShader
{
public:
    OMShader() = default;
    OMShader(OMShaderFileType type, std::vector<uint8_t> data, std::string filename, std::string entrypoint, OMShaderType typebase): type(type), data(data), entrypoint(entrypoint), filename(filename), typebase(typebase)
    {
    }
    ~OMShader() = default;

    std::shared_ptr<OMShader> convertTo(OMShaderFileType type);

    const std::vector<uint8_t> data;
    OMShaderFileType type;
    std::string filename;
    std::string entrypoint;
    OMShaderType typebase;
};
};

#endif