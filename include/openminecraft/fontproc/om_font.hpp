#ifndef OM_FONT_HPP
#define OM_FONT_HPP
#include <iosfwd>
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

    log::OMLogger logger;
};
} // namespace openminecraft::fontproc

#endif