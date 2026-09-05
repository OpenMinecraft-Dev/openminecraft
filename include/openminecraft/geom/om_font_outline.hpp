#ifndef OM_FONT_OUTLINE_HPP
#define OM_FONT_OUTLINE_HPP
#include "openminecraft/geom/om_font.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace openminecraft::geom
{
enum OMFontOutlineOperationType
{
    Move,
    Line,
    Quadratic,
    Cubic,
    Close
};

struct OMFontOutlineOperation
{
    OMFontOutlineOperationType type;
    glm::vec2 target;
    glm::vec2 control1;
    glm::vec2 control2;
};

class OMFontOutline
{
  public:
    OMFontOutline() = default;
    ~OMFontOutline() = default;

    void moveTo(glm::vec2 vec)
    {
        operations.push_back({Move, vec});
    }

    void lineTo(glm::vec2 vec)
    {
        operations.push_back({Line, vec});
    }

    void quadraticTo(glm::vec2 vec, glm::vec2 ct1)
    {
        operations.push_back({Quadratic, vec, ct1});
    }

    void cubicTo(glm::vec2 vec, glm::vec2 ct1, glm::vec2 ct2)
    {
        operations.push_back({Cubic, vec, ct1, ct2});
    }

    void closePath()
    {
        operations.push_back({Close});
    }

    std::vector<OMFontOutlineOperation> operations;
};
} // namespace openminecraft::geom

template <> struct fmt::formatter<openminecraft::geom::OMFontOutlineOperationType> : formatter<string_view>
{
    auto format(openminecraft::geom::OMFontOutlineOperationType c, format_context &ctx) const
        -> format_context::iterator
    {
        std::string s;
        switch (c)
        {
        case openminecraft::geom::Move:
            s = "Move";
            break;
        case openminecraft::geom::Line:
            s = "Line";
            break;
        case openminecraft::geom::Quadratic:
            s = "Quadratic";
            break;
        case openminecraft::geom::Cubic:
            s = "Cubic";
            break;
        case openminecraft::geom::Close:
            s = "Close";
            break;
        default:
            s = "Unknown";
            break;
        }
        return formatter<string_view>::format(s, ctx);
    }
};

#endif
