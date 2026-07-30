#ifndef OM_FONT_HPP
#define OM_FONT_HPP
#include "glm/ext/vector_float2.hpp"
#include "openminecraft/fontproc/om_font_glyph.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <vector>

namespace openminecraft::fontproc
{
constexpr const char allocatorId[] = "font_processor";
class OMFontOutline;
class OMFont;
struct OMFontShapeResult
{
    uint32_t glyphId, cluster;
    float offsetx;
    float offsety;
    float advancex;
    float advancey;
    OMFont *font;
};
class OMFont
{
  public:
    OMFont(std::istream &istr);
    ~OMFont();

    auto scale() -> glm::vec2;
    auto buildOutline(int charcode, bool uni = true) -> OMFontOutline;
    auto fetchBox(int charcode, bool uni = true) -> glm::vec4;
    auto buildBasicPolygon(int charcode, bool uni = true) -> std::shared_ptr<OMTriangleList>;
    auto buildGlyph(int charcode, bool uni = true) -> std::shared_ptr<OMFontGlyph>;
    auto shape(std::string s) -> std::vector<OMFontShapeResult>;

  private:
    void *hbFont;
    void *hbFace;

    log::OMLogger logger;
};
} // namespace openminecraft::fontproc

#endif
