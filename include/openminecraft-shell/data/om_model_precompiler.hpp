#ifndef OM_MODEL_PRECOMPILER_HPP
#define OM_MODEL_PRECOMPILER_HPP

#include "glm/fwd.hpp"
#include "openminecraft-shell/data/om_identifier.hpp"
#include "openminecraft-shell/data/om_textureatlas.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/io/json/om_io_ast_json.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/wrap/om_renderer_voxel.hpp"
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
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

enum OMModelAxis : uint8_t
{
    X = 0,
    Y = 1,
    Z = 2
};

struct OMModelFace
{
    OMModelCullSide cull;
    int textureid;
    glm::vec2 uv0, uv1;
    int rotation;
    bool secondaryTex;
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

    glm::vec3 from, to;
    bool shade;
    bool rotate;
    glm::vec3 rotateOrigin;
    OMModelAxis rotateAxis;
    double rotateAngle;
};
class OMModelPrecompiler : public openminecraft::renderer::common::wrap::OMVoxelHandler
{
  public:
    OMModelPrecompiler(std::string root, OMTextureAtlas *);
    ~OMModelPrecompiler() = default;

    auto precompile(OMIdentifier, bool = true) -> std::shared_ptr<openminecraft::io::json::OMJsonNode>;
    auto wrapFace(std::shared_ptr<openminecraft::io::json::OMJsonNode>, OMModelCullSide, glm::vec3 from, glm::vec3 to)
        -> OMModelFace;
    auto wrapPart(std::shared_ptr<openminecraft::io::json::OMJsonNode>) -> OMModelPart;

    auto loadModelPart(OMIdentifier, bool = true) -> int;
    auto loadModelPartWithArgs(OMIdentifier, int, int, int, bool, bool = true) -> int;

    auto queryNumParts(int bsid) -> int override;
    auto queryPartFaceEnabled(int bsid, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing) -> bool override;
    auto queryPartFaceTex(int bsid, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing) -> int override;
    auto queryPartFaceCull(int bsid, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing)
        -> openminecraft::renderer::common::wrap::OMVoxelFacing override;
    auto queryPartAABB(int bsid, int pid) -> renderer::common::wrap::OMVoxelAABB override;
    auto queryPartFaceUV(int bsid, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing) -> glm::vec4 override;
    auto queryPartFaceRotation(int bsid, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing) -> int override;
    auto queryPartRotationAxis(int bsid, int pid) -> int override;
    auto queryPartRotationCenter(int bsid, int pid) -> glm::vec3 override;
    auto queryPartRotationAngle(int bsid, int pid) -> int override;
    auto querySoild(int bsid) -> bool override;
    auto queryOcclusionShape(int bsid) -> openminecraft::renderer::common::wrap::OMVoxelShape override;
    auto queryAmbientOcclusion(int bsid) -> bool override;
    auto queryPartShade(int bsid, int pid) -> bool override;
    auto queryComplex(int bsid) -> bool override;
    auto queryPartFaceSecondaryTexture(int bsid, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing)
        -> bool override;
    auto queryPartRotationAngleF(int bsid, int pid) -> float override;

  private:
    int modelId = 0;
    std::string root;
    openminecraft::log::OMLogger logger;
    OMTextureAtlas &textureAtlas;
    std::vector<std::vector<OMModelPart>> models;
    std::vector<bool> modelSoild;
    std::vector<bool> modelAmbientOcculusion;
    std::vector<bool> modelComplex;
};
} // namespace openminecraftshell::data

#endif