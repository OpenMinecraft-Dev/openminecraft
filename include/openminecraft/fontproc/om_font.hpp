#ifndef OM_FONT_HPP
#define OM_FONT_HPP
#include "openminecraft/fontproc/om_font_glyph.hpp"
#include "openminecraft/fontproc/om_font_triangle_list.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <iosfwd>
#include <memory>

namespace openminecraft::fontproc
{
constexpr const char allocatorId[] = "font_processor";

class OMFont
{
  public:
    OMFont(std::istream &istr);
    ~OMFont();

    std::shared_ptr<OMTriangleList> buildBasicPolygon(int charcode);
    std::shared_ptr<OMFontGlyph> buildGlyph(int charcode);

  private:
    void *hbFont;
    void *hbFace;

    log::OMLogger logger;
};
} // namespace openminecraft::fontproc

#endif
