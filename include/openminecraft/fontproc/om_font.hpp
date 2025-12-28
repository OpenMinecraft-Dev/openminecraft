#ifndef OM_FONT_HPP
#define OM_FONT_HPP
#include <iosfwd>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace openminecraft::fontproc
{
class OMFont
{
public:
    OMFont(std::istream &istr);

private:
    void *hbFont;
    void *hbFace;
    FT_Library ftLibrary;
    FT_Face ftFace;
};
}

#endif