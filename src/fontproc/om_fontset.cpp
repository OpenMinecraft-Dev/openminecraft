#include "openminecraft/fontproc/om_fontset.hpp"

namespace openminecraft::fontproc
{
void OMFontSet::shape(std::string s)
{
    for (auto f : fontList)
    {
        logger.info("shaping with font {}", (void *)f.get());
        auto r = f->shape(s);
        for (auto &c : r)
        {
            logger.info("Cluster {} glyphid 0x{:x} + offset({}, {}) / advance({}, {})", c.cluster, c.glyphId, c.offsetx,
                        c.offsety, c.advancex, c.advancey);
        }
    }
}
} // namespace openminecraft::fontproc
