#ifndef OM_FONT_GLYPH_HPP
#define OM_FONT_GLYPH_HPP

#include <utility>

#include "glm/glm.hpp"
#include "openminecraft/fontproc/om_font_triangle_list.hpp"

namespace openminecraft::fontproc
{
class OMFontGlyph
{
  public:
    OMFontGlyph(std::shared_ptr<OMTriangleList> triangleList, glm::vec4 extent)
        : triangleList(std::move(triangleList)), extent(extent)
    {
    }

    std::shared_ptr<OMTriangleList> triangleList;
    glm::vec4 extent;
};
} // namespace openminecraft::fontproc

#endif
