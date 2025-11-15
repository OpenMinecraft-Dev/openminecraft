#ifndef OM_RENDERER_LAYER_HPP
#define OM_RENDERER_LAYER_HPP

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include <memory>
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
    OMRenderer(AppInfo info, void *window);
    virtual ~OMRenderer() = default;

    virtual std::string driver() = 0;
    virtual std::shared_ptr<common::OMRendererBuffer> allocateVertexBuffer(uint64_t length) = 0;

  protected:
    void *window;

  private:
    const AppInfo info;
};
} // namespace openminecraft::renderer

#endif