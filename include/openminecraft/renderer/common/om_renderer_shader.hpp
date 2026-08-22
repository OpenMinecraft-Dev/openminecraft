#ifndef OM_RENDERER_SHADER_HPP
#define OM_RENDERER_SHADER_HPP
#include "openminecraft/renderer/om_renderer_object.hpp"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace openminecraft::renderer::common
{
enum OMShaderFileType
{
    GLSLSource,
    HLSLSource,
    SPIRVBinary,
    GLNative
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

class OMShader : public OMRendererObject
{
  public:
    OMShader() = default;
    OMShader(OMShaderFileType type, std::vector<uint8_t> data, std::string filename, std::string entrypoint,
             OMShaderType typebase)
        : data(std::move(data)), type(type), filename(std::move(filename)), entrypoint(std::move(entrypoint)),
          typebase(typebase)
    {
    }
    virtual ~OMShader() = default;

    const std::vector<uint8_t> data;
    OMShaderFileType type;
    std::string filename;
    std::string entrypoint;
    OMShaderType typebase;

    inline auto objType() -> OMRendererObjectType override
    {
        return Shader;
    }
};
}; // namespace openminecraft::renderer::common

#endif
