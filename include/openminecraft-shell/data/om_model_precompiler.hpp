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
#include <unordered_map>
#include <vector>
namespace openminecraft::shell::data
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
    Z = 2,
    Any = 3
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

    glm::vec3 rotateAngleExt;
};
struct OMModel
{
    std::vector<OMModelPart> parts;
    std::vector<bool> partAmbientOcculusion;
    std::vector<bool> partComplex;
    bool soild;
    bool translucent;
    bool fluid;
    bool fluidFalling;
    uint8_t fluidLevel;
    uint32_t fluidTex;
    uint32_t fluidTexFlow;
    bool waterlogged;
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

    auto composeModel(std::vector<int>, bool = true, bool = false, bool = false) -> int;
    auto composeFluidModel(OMIdentifier ident, bool translucent, bool falling, uint8_t level, OMIdentifier tex,
                           OMIdentifier texFlow) -> int;

    auto loadModelPart(OMIdentifier) -> int;
    auto loadModelPartWithArgs(OMIdentifier, int, int, int, bool) -> int;

    auto querySkipsRendering(int modelId, int targetModelId, openminecraft::renderer::common::wrap::OMVoxelFacing)
        -> bool override;
    auto queryNumParts(int modelId) -> int override;
    auto queryPartFaceEnabled(int modelId, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing)
        -> bool override;
    auto queryPartFaceTex(int modelId, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing) -> int override;
    auto queryPartFaceCull(int modelId, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing)
        -> openminecraft::renderer::common::wrap::OMVoxelFacing override;
    auto queryPartAABB(int modelId, int pid) -> openminecraft::renderer::common::wrap::OMVoxelAABB override;
    auto queryPartFaceUV(int modelId, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing)
        -> glm::vec4 override;
    auto queryPartFaceRotation(int modelId, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing)
        -> int override;
    auto queryPartRotationAxis(int modelId, int pid) -> int override;
    auto queryPartRotationCenter(int modelId, int pid) -> glm::vec3 override;
    auto queryPartRotationAngle(int modelId, int pid) -> int override;
    auto querySoild(int modelId) -> bool override;
    auto queryOcclusionShape(int modelId) -> openminecraft::renderer::common::wrap::OMVoxelShape override;
    auto queryPartAmbientOcclusion(int modelId, int pid) -> bool override;
    auto queryPartShade(int modelId, int pid) -> bool override;
    auto queryPartComplex(int modelId, int pid) -> bool override;
    auto queryPartFaceSecondaryTexture(int modelId, int pid, openminecraft::renderer::common::wrap::OMVoxelFacing)
        -> bool override;
    auto queryPartRotationAngleF(int modelId, int pid) -> float override;
    auto queryPartRotationAngleExt(int modelId, int pid) -> glm::vec3 override;
    auto queryTranslucent(int modelId) -> bool override;
    auto queryFluid(int modelId) -> bool override;
    auto queryFluidFalling(int modelId) -> bool override;
    auto queryFluidLevel(int modelId) -> uint8_t override;
    auto queryFluidTex(int modelId) -> int override;
    auto queryWaterlogged(int modelId) -> bool override;
    auto queryFluidSame(int modelId1, int modelId2) -> bool override;
    auto queryFluidTexSec(int modelId) -> int override;

  private:
    int modelId = 0;
    std::string root;
    openminecraft::log::OMLogger logger;
    OMTextureAtlas &textureAtlas;
    std::vector<OMModel> blockModels;
    std::vector<std::vector<OMModelPart>> modelParts;
    std::unordered_map<OMIdentifier, int> modelPartIdents;
    std::vector<bool> modelAmbientOcculusion;
    std::vector<bool> modelComplex;

    std::unordered_map<int, OMIdentifier> fluidModels;
};
} // namespace openminecraft::shell::data

#endif