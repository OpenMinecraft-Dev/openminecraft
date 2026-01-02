#ifndef OM_FONT_OUTLINE_HPP
#define OM_FONT_OUTLINE_HPP
#include <fmt/format.h>
#include <freetype/ftimage.h>
#include <glm/glm.hpp>
#include <vector>

namespace openminecraft::fontproc
{
enum OMFontOutlineOperationType
{
    Move,
    Line,
    Conic,
    Cubic
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
    OMFontOutline()
    {
    }
    ~OMFontOutline()
    {
    }

    void moveTo(const FT_Vector *vec)
    {
        operations.push_back({Move, {static_cast<float>(vec->x), static_cast<float>(vec->y)}});
    }

    void lineTo(const FT_Vector *vec)
    {
        operations.push_back({Line, {static_cast<float>(vec->x), static_cast<float>(vec->y)}});
    }

    void conicTo(const FT_Vector *vec, const FT_Vector *ct1)
    {
        operations.push_back({Conic,
                              {static_cast<float>(vec->x), static_cast<float>(vec->y)},
                              {static_cast<float>(ct1->x), static_cast<float>(ct1->y)}});
    }

    void cubicTo(const FT_Vector *vec, const FT_Vector *ct1, const FT_Vector *ct2)
    {
        operations.push_back({Cubic,
                              {static_cast<float>(vec->x), static_cast<float>(vec->y)},
                              {static_cast<float>(ct1->x), static_cast<float>(ct1->y)},
                              {static_cast<float>(ct2->x), static_cast<float>(ct2->y)}});
    }

    std::vector<OMFontOutlineOperation> operations;
};
} // namespace openminecraft::fontproc

template <> struct fmt::formatter<openminecraft::fontproc::OMFontOutlineOperationType> : formatter<string_view>
{
    auto format(openminecraft::fontproc::OMFontOutlineOperationType c, format_context &ctx) const
        -> format_context::iterator
    {
        std::string s;
        switch (c)
        {
        case openminecraft::fontproc::Move:
            s = "Move";
            break;
        case openminecraft::fontproc::Line:
            s = "Line";
            break;
        case openminecraft::fontproc::Conic:
            s = "Conic";
            break;
        case openminecraft::fontproc::Cubic:
            s = "Cubic";
            break;
        default:
            s = "Unknown";
            break;
        }
        return formatter<string_view>::format(s, ctx);
    }
};

#endif