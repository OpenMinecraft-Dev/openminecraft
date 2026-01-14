#ifndef OM_FONT_TRIANGLE_LIST
#define OM_FONT_TRIANGLE_LIST

#include "glm/glm.hpp"
#include "mapbox/earcut.hpp"
#include "openminecraft/fontproc/om_font_polygon.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace mapbox::util
{
template <> struct nth<0, glm::vec2>
{
    inline static auto get(const glm::vec2 &t)
    {
        return t.x;
    };
};
template <> struct nth<1, glm::vec2>
{
    inline static auto get(const glm::vec2 &t)
    {
        return t.y;
    };
};
} // namespace mapbox::util

namespace openminecraft::fontproc
{
class OMTriangleList
{
  public:
    OMTriangleList(std::shared_ptr<OMFontPolygon> mainPoly, std::vector<std::shared_ptr<OMFontPolygon>> holes)
        : logger("OMTriangleList", this)
    {
        std::vector<std::vector<glm::vec2>> polybase;
        polybase.push_back(mainPoly->vertices);
        vertices.insert(vertices.end(), mainPoly->vertices.begin(), mainPoly->vertices.end());
        for (auto ph : holes)
        {
            polybase.push_back(ph->vertices);
            vertices.insert(vertices.end(), mainPoly->vertices.begin(), mainPoly->vertices.end());
        }
        indices = mapbox::earcut<uint32_t>(polybase);
    }
    // geopeila: merge several different triangle lists
    OMTriangleList(std::vector<std::shared_ptr<OMTriangleList>> lists) : logger("OMTriangleList", this)
    {
        uint32_t offset = 0;
        for (auto l : lists)
        {
            vertices.insert(vertices.end(), l->vertices.begin(), l->vertices.end());
            for (auto i : l->indices)
            {
                indices.push_back(i + offset);
            }
            offset += l->vertices.size();
        }
    }
    ~OMTriangleList() = default;

    std::vector<uint32_t> indices;
    std::vector<glm::vec2> vertices;

    log::OMLogger logger;
};
} // namespace openminecraft::fontproc

#endif
