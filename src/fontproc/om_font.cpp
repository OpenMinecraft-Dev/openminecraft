#include "openminecraft/fontproc/om_font.hpp"
#include "harfbuzz/hb.h"

#include "openminecraft/io/om_io_utils.hpp"

namespace openminecraft::fontproc
{
OMFont::OMFont(std::istream &istr)
{
    auto temp = io::readOnce(&istr);

    auto blob = hb_blob_create(reinterpret_cast<const char *>(temp.data()), temp.size(), HB_MEMORY_MODE_READONLY, nullptr, nullptr);
    hbFace = hb_face_create(blob, 0);
    hbFont = hb_font_create(static_cast<hb_face_t *>(hbFace));
}

}