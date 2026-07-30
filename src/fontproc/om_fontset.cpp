#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include <vector>

namespace openminecraft::fontproc
{
void OMFontSet::shape(std::string s)
{
    std::vector<OMFontShapeResult> glyphs = {};
    for (auto f : fontList)
    {
        logger.info("shaping with font {}", (void *)f.get());
        auto r = f->shape(s);

        if (glyphs.size() < r.size())
        {
            glyphs.resize(r.size());
        }

        for (int i = 0; i < r.size(); ++i)
        {
            if (glyphs[i].glyphId == 0x0 && r[i].glyphId != 0x0)
            {
                glyphs[i] = r[i];
            }
        }
    }

    for (auto &c : glyphs)
    {
        logger.info("{} Cluster {} glyphid 0x{:x} + offset({}, {}) / advance({}, {})", (void *)c.font, c.cluster,
                    c.glyphId, c.offsetx, c.offsety, c.advancex, c.advancey);
    }
}
} // namespace openminecraft::fontproc
