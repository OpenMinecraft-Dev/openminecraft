#ifndef OM_FONTSET_HPP
#define OM_FONTSET_HPP

#include "openminecraft/fontproc/om_font.hpp"
#include <string>
#include <vector>
namespace openminecraft::fontproc
{
class OMFontSet
{
  public:
    OMFontSet() = default;
    ~OMFontSet() = default;

    auto shape(std::string s) -> void;

    std::vector<std::shared_ptr<OMFont>> fontList;
};
} // namespace openminecraft::fontproc

#endif
