#ifndef OM_FONT_POLYGON_HPP
#define OM_FONT_POLYGON_HPP
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace openminecraft::fontproc
{
class OMFontPolygon
{
  public:
    OMFontPolygon() = default;
    ~OMFontPolygon() = default;

    void addVertex(glm::vec2 vec)
    {
        vertices.push_back(vec);
    }

    auto area() -> double
    {
        double area = 0;
        for (int i = 0; i < vertices.size(); i++)
        {
            auto vi = vertices[i];
            auto vj = vertices[(i + 1) % vertices.size()];
            area += vi.x * vj.y - vi.y * vj.x;
        }

        return std::fabs(area) / 2;
    }

    auto isPointInside(glm::vec2 point) -> bool
    {
        int count = 0;
        for (int i = 0; i < vertices.size(); i++)
        {
            auto p1 = vertices[i];
            auto p2 = vertices[(i + 1) % vertices.size()];
            if ((p1.y > point.y) != (p2.y > point.y) &&
                (point.x < (p2.x - p1.x) * (point.y - p1.y) / (p2.y - p1.y) + p1.x))
            {
                count++;
            }
        }
        return count % 2 == 1;
    }

    auto isPolyInside(std::shared_ptr<OMFontPolygon> p) -> bool
    {
        for (auto pp : p->vertices)
        {
            if (!isPointInside(pp))
            {
                return false;
            }
        }
        return true;
    }

    std::vector<glm::vec2> vertices;
};
} // namespace openminecraft::fontproc

#endif
