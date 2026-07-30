#ifndef OM_FONTSET_HPP
#define OM_FONTSET_HPP

#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <string>
#include <vector>
namespace openminecraft::fontproc
{
class OMFontSet
{
  public:
    OMFontSet() : logger("OMFontSet", this)
    {
    }
    ~OMFontSet() = default;

    auto shape(std::string s) -> void;

    std::vector<std::shared_ptr<OMFont>> fontList;

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::fontproc

#endif
