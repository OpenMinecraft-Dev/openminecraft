#ifndef OM_FONT_HPP
#define OM_FONT_HPP
#include <ft2build.h>
#include <iosfwd>
#include FT_FREETYPE_H
#include "openminecraft/log/om_log_common.hpp"

namespace openminecraft::fontproc
{
class OMFont
{
  public:
    OMFont(std::istream &istr);
    ~OMFont();

    void parseChar(int charcode);

  private:
    void *hbFont;
    void *hbFace;
    FT_Library ftLibrary;
    FT_Face ftFace;

    log::OMLogger logger;
};
} // namespace openminecraft::fontproc

#endif