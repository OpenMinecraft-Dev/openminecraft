#ifndef OM_VERTEX_FORMAT_HPP
#define OM_VERTEX_FORMAT_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <string>
#include <utility>
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
    Vec4d,
    Boolean,
    Vec2b,
    Vec3b,
    Vec4b,
    Mat4x4,
    Mat3x3
};

struct OMVertexFormatGroup
{
    bool isInstance = false;
    int binding = 0;
    std::vector<std::tuple<std::string, OMVertexPropType, int>> parts;
};

class OMVertexFormat
{
  public:
    OMVertexFormat();
    ~OMVertexFormat();

    void appendPart(std::string, OMVertexPropType);
    void debugState();
    void decideStruct();
    static int typeSize(OMVertexPropType);
    static int typeAlign(OMVertexPropType);

    void setInstance();
    void nextGroup();

    std::vector<OMVertexFormatGroup> parts;
    OMVertexFormatGroup currentGroup;

  private:
    int binding = 0;
    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common::basics

#endif
