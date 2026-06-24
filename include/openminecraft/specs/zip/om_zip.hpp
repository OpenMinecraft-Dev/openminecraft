#ifndef OM_ZIP_HPP
#define OM_ZIP_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <array>
#include <istream>
#include <memory>
namespace openminecraft::specs::zip
{
constexpr std::array<char, 4> header = {0x06, 0x05, 0x4B, 0x50};
class OMZip
{
  public:
    OMZip(std::shared_ptr<std::istream> istr);

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::specs::zip

#endif
