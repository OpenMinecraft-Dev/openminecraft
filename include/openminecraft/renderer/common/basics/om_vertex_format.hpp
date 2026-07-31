#ifndef OM_VERTEX_FORMAT_HPP
#define OM_VERTEX_FORMAT_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <string>
#include <vector>
namespace openminecraft::renderer::common::basics
{
enum OMVertexPropType
{
    Float,
    Vec2f,
    Vec3f,
    Vec4f,
    Integer,
    Vec2i,
    Vec3i,
    Vec4i,
    Double,
    Vec2d,
    Vec3d,
    Vec4d
};

struct OMVertexFormatGroup
{
    bool isInstance = false;
    int binding = 0;
    std::vector<std::tuple<std::string, OMVertexPropType, int>> parts;
    int size;
};

class OMVertexFormat
{
  public:
    OMVertexFormat();
    ~OMVertexFormat();

    auto appendPart(std::string, OMVertexPropType) -> OMVertexFormat *;
    auto debugState() -> OMVertexFormat *;
    auto decideStruct() -> OMVertexFormat *;
    static auto typeSize(OMVertexPropType) -> int;
    static auto typeAlign(OMVertexPropType) -> int;

    auto setInstance() -> OMVertexFormat *;
    auto nextGroup() -> OMVertexFormat *;

    std::vector<OMVertexFormatGroup> parts;
    OMVertexFormatGroup currentGroup;

  private:
    int binding = 0;
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common::basics

#endif
