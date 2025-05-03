#ifndef OM_RENDERER_LAYER_HPP
#define OM_RENDERER_LAYER_HPP

#include "openminecraft/util/om_util_version.hpp"
#include <string>

namespace openminecraft::renderer
{
struct AppInfo
{
    std::string appName;
    util::Version appVer;
    std::string engineName;
    util::Version engineVer;
    util::Version minApiVersion;
};
class OMRenderer
{
  public:
    OMRenderer(AppInfo info);
    ~OMRenderer();

    virtual std::string driver() = 0;

  private:
    const AppInfo info;
};
} // namespace openminecraft::renderer

#endif