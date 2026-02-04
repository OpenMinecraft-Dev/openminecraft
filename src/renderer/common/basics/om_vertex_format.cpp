#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"

namespace openminecraft::renderer::common::basics
{
OMVertexFormat::OMVertexFormat() : logger("OMVertexFormat", this)
{
}
OMVertexFormat::~OMVertexFormat()
{
}

void OMVertexFormat::appendPart(std::string id, OMVertexPropType type)
{
    parts.push_back(std::make_tuple(id, type, 0));
}

void OMVertexFormat::decideStruct()
{
    int offset = 0;
    for (auto &p : parts)
    {
        auto align = typeAlign(std::get<OMVertexPropType>(p));
        if (offset % align != 0)
        {
            offset += align - (offset % align);
        }
        int &off = std::get<int>(p);
        off = offset;
        offset += typeSize(std::get<OMVertexPropType>(p));
    }
}

void OMVertexFormat::debugState()
{
    for (auto &o : parts)
    {
        logger.info("{}: size {} @ +{}", std::get<std::string>(o), typeSize(std::get<OMVertexPropType>(o)),
                    std::get<int>(o));
    }
}

int OMVertexFormat::typeAlign(OMVertexPropType type)
{
    switch (type)
    {
    case Double:
    case Vec2d:
    case Vec3d:
    case Vec4d:
        return sizeof(double);
    case Float:
    case Integer:
    case Vec2f:
    case Vec2i:
    case Vec3f:
    case Vec3i:
    case Vec4f:
    case Vec4i:
        return sizeof(float);
    case Boolean:
    case Vec2b:
    case Vec3b:
    case Vec4b:
        return sizeof(bool);
    }
}

int OMVertexFormat::typeSize(OMVertexPropType type)
{
    switch (type)
    {
    case Float:
        return sizeof(float);
    case Integer:
        return sizeof(int);
    case Double:
        return sizeof(double);
    case Vec2f:
        return sizeof(float) * 2;
    case Vec3f:
        return sizeof(float) * 3;
    case Vec4f:
        return sizeof(float) * 4;
    case Vec2i:
        return sizeof(int) * 2;
    case Vec3i:
        return sizeof(int) * 3;
    case Vec4i:
        return sizeof(int) * 4;
    case Vec2d:
        return sizeof(double) * 2;
    case Vec3d:
        return sizeof(double) * 3;
    case Vec4d:
        return sizeof(double) * 4;
    case Boolean:
        return sizeof(bool);
    case Vec2b:
        return sizeof(bool) * 2;
    case Vec3b:
        return sizeof(bool) * 3;
    case Vec4b:
        return sizeof(bool) * 4;
    default:
        return 0;
    }
}
} // namespace openminecraft::renderer::common::basics
