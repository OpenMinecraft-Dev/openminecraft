#ifndef OM_FONT_HPP
#define OM_FONT_HPP
#include "glm/ext/vector_float2.hpp"
#include "openminecraft/fontproc/om_font_glyph.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <iosfwd>
#include <memory>

namespace openminecraft::fontproc
{
constexpr const char allocatorId[] = "font_processor";
class OMFontOutline;
class OMFont
{
  public:
    OMFont(std::istream &istr);
    ~OMFont();

    auto scale() -> glm::vec2;
    auto buildOutline(int charcode, bool uni = true) -> OMFontOutline;
    auto buildBasicPolygon(int charcode, bool uni = true) -> std::shared_ptr<OMTriangleList>;
    auto buildGlyph(int charcode, bool uni = true) -> std::shared_ptr<OMFontGlyph>;
    auto shape(std::string s) -> void;

  private:
    void *hbFont;
    void *hbFace;

    log::OMLogger logger;
};
} // namespace openminecraft::fontproc

#endif
