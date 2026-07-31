#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"

namespace openminecraft::renderer::common::basics
{
OMVertexFormat::OMVertexFormat() : logger("OMVertexFormat", this)
{
}
OMVertexFormat::~OMVertexFormat() = default;

auto OMVertexFormat::appendPart(std::string id, OMVertexPropType type) -> OMVertexFormat *
{
    currentGroup.parts.emplace_back(id, type, 0);
    return this;
}

auto OMVertexFormat::decideStruct() -> OMVertexFormat *
{
    for (auto &part : parts)
    {
        int offset = 0;
        for (auto &p : part.parts)
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
        part.size = offset;
    }
    return this;
}

auto OMVertexFormat::debugState() -> OMVertexFormat *
{
    for (auto &p : parts)
    {
        logger.info("binding {}, instance = {}", p.binding, p.binding ? "true" : "false");
        for (auto &o : p.parts)
        {
            logger.info("{}: size {} @ +{}", std::get<std::string>(o), typeSize(std::get<OMVertexPropType>(o)),
                        std::get<int>(o));
        }
    }
    return this;
}

auto OMVertexFormat::setInstance() -> OMVertexFormat *
{
    currentGroup.isInstance = true;
    return this;
}

auto OMVertexFormat::nextGroup() -> OMVertexFormat *
{
    parts.push_back(currentGroup);
    currentGroup = OMVertexFormatGroup();
    binding++;
    currentGroup.binding = binding;
    return this;
}

auto OMVertexFormat::typeAlign(OMVertexPropType type) -> int
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
    default:
        return sizeof(float);
    }
}

auto OMVertexFormat::typeSize(OMVertexPropType type) -> int
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
    default:
        return 0;
    }
}
} // namespace openminecraft::renderer::common::basics
