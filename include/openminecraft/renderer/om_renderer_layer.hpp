#ifndef OM_RENDERER_LAYER_HPP
#define OM_RENDERER_LAYER_HPP

#include "common/om_renderer_texture.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/util/om_util_version.hpp"
#include <glm/glm.hpp>
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
    virtual common::OMRendererBuffer *allocateBuffer(common::OMBufferUsage usage, uint64_t length) = 0;
    virtual common::OMRendererTexture *allocateTexture(uint64_t width, uint64_t height, common::OMTextureType type,
                                                       common::OMTextureArrangement arr) = 0;
    virtual glm::vec2 getExtent() = 0;

  protected:
    void *window;

  private:
    const AppInfo info;
};
} // namespace openminecraft::renderer

#endif
