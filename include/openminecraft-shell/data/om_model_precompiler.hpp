#ifndef OM_MODEL_PRECOMPILER_HPP
#define OM_MODEL_PRECOMPILER_HPP

#include "glm/fwd.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/data/om_textureatlas.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <memory>
#include <string>
namespace openminecraftshell::data
{
enum OMModelCullSide
{
    Down,
    Up,
    South,
    North,
    West,
    East,
    None,
};
static auto from(std::string s) -> OMModelCullSide
{
    using namespace binary::hash;
    switch (hash_compile_time(s.c_str()))
    {
    case "down"_hash:
        return Down;
    case "up"_hash:
        return Up;
    case "south"_hash:
        return South;
    case "north"_hash:
        return North;
    case "east"_hash:
        return East;
    case "west"_hash:
        return West;
    default:
        return None;
    }
}
enum OMModelAxis
{
    X,
    Y,
    Z
};
static auto fromAxis(std::string s) -> OMModelAxis
{
    using namespace binary::hash;
    switch (hash_compile_time(s.c_str()))
    {
    default:
    case "x"_hash:
        return X;
    case "y"_hash:
        return Y;
    case "z"_hash:
        return Z;
    }
}
struct OMModelFace
{
    OMModelCullSide cull;
    int textureid;
    glm::ivec2 uv0, uv1;
    bool autoUV;
    bool keepUV;
    int rotation;
};
struct OMModelPart
{
    OMModelFace east;
    bool enableEast;
    OMModelFace west;
    bool enableWest;
    OMModelFace down;
    bool enableDown;
    OMModelFace up;
    bool enableUp;
    OMModelFace south;
    bool enableSouth;
    OMModelFace north;
    bool enableNorth;

    glm::ivec3 from, to;
    bool shade;
    bool rotate;
    glm::ivec3 rotateOrigin;
    OMModelAxis rotateAxis;
    double rotateAngle;
};
class OMModelPrecompiler
{
  public:
    OMModelPrecompiler(std::string root, OMTextureAtlas &);
    ~OMModelPrecompiler() = default;

    auto precompile(OMIdentifier, bool = true) -> std::shared_ptr<openminecraft::io::json::OMJsonNode>;
    auto wrapFace(std::shared_ptr<openminecraft::io::json::OMJsonNode>) -> OMModelFace;
    auto wrapPart(std::shared_ptr<openminecraft::io::json::OMJsonNode>) -> OMModelPart;

    auto loadModel(OMIdentifier) -> int;

  private:
    int modelId = 0;
    std::string root;
    openminecraft::log::OMLogger logger;
    OMTextureAtlas &textureAtlas;
    std::vector<std::vector<OMModelPart>> models;
};
} // namespace openminecraftshell::data

#endif