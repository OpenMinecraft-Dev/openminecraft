#ifndef OM_FONT_POLYGON_HPP
#define OM_FONT_POLYGON_HPP
#include <glm/glm.hpp>
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

    double area()
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

    std::vector<glm::vec2> vertices;
};
} // namespace openminecraft::fontproc

#endif