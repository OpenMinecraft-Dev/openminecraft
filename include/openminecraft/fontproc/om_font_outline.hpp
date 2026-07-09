#ifndef OM_FONT_OUTLINE_HPP
#define OM_FONT_OUTLINE_HPP
#include "om_font_polygon.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/mem/om_mem_stl_allocator.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace openminecraft::fontproc
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

    auto buildPolygons(int prec, int xsc, int ysc) -> std::vector<std::shared_ptr<OMFontPolygon>>
    {
        std::vector<std::shared_ptr<OMFontPolygon>> polygons;
        auto poly = mem::fast_shared<allocatorId, OMFontPolygon>();
        glm::vec2 current;
        for (auto op : operations)
        {
            switch (op.type)
            {
            case Close:
                polygons.push_back(poly);
                poly = mem::fast_shared<allocatorId, OMFontPolygon>();
                break;
            case Move:
            case Line:
                poly->addVertex(op.target);
                current = op.target;
                break;
            case Quadratic:
                for (int i = 1; i <= prec; i++)
                {
                    auto add = static_cast<float>(i) / static_cast<float>(prec);
                    auto pp =
                        current * (1 - add) * (1 - add) + op.control1 * (2 * add * (1 - add)) + op.target * add * add;
                    poly->addVertex(pp);
                }
                current = op.target;
                break;
            case Cubic:
                for (int i = 1; i <= prec; i++)
                {
                    auto add = static_cast<float>(i) / static_cast<float>(prec);
                    auto pp = current * (1 - add) * (1 - add) * (1 - add) +
                              op.control1 * (3 * add * (1 - add) * (1 - add)) +
                              op.control2 * (3 * add * add * (1 - add)) + op.target * add * add * add;
                    poly->addVertex(pp);
                }
                current = op.target;
                break;
            }
        }

        for (auto pp : polygons)
        {
            for (auto &p : pp->vertices)
            {
                p /= glm::vec2{xsc, ysc};
            }
        }

        return polygons;
    }
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
        case openminecraft::fontproc::Quadratic:
            s = "Quadratic";
            break;
        case openminecraft::fontproc::Cubic:
            s = "Cubic";
            break;
        case openminecraft::fontproc::Close:
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
