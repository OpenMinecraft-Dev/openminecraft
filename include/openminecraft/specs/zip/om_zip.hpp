#ifndef OM_ZIP_HPP
#define OM_ZIP_HPP

#include "openminecraft/log/om_log_common.hpp"
#include <istream>
#include <memory>
namespace openminecraft::specs::zip
{
class OMZip
{
  public:
    OMZip(std::shared_ptr<std::istream> istr);

  private:
    log::OMLogger logger;
};
} // namespace openminecraft::specs::zip

#endif
