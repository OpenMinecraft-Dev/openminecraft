#include "openminecraft/fontproc/om_fontset.hpp"

namespace openminecraft::fontproc
{
void OMFontSet::shape(std::string s)
{
    for (auto f : fontList)
    {
        f->shape(s);
    }
}
} // namespace openminecraft::fontproc
