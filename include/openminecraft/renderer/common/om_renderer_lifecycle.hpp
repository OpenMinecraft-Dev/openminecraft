#ifndef OM_RENDERER_LIFECYCLE_HPP
#define OM_RENDERER_LIFECYCLE_HPP

#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/om_renderer_object.hpp"
#include <cstdint>
#include <functional>
#include <vector>

namespace openminecraft::renderer::common
{
class OMRendererBuffer;
class OMRendererTexture;
class OMRendererLifeCycle;

struct OMRendererResourceTag
{
    uint32_t frameIndex = 0;
    uint64_t frameSerial = 0;
};

// Owns resources after they are unregistered from the renderer registry.
// Resources are deleted only after their frame tag is complete; textures also
// require their transfer upload to be complete.
class OMRendererLifeCycle : public OMRendererObject
{
  public:
    OMRendererLifeCycle();
    using FrameComplete = std::function<bool(uint32_t frameIndex, uint64_t frameSerial)>;

    void setFrameTag(uint32_t frameIndex, uint64_t frameSerial)
    {
        currentTag = {frameIndex, frameSerial};
    }

    void retireBuffer(OMRendererBuffer *buffer);
    void retireTexture(OMRendererTexture *texture);

    void collect(const FrameComplete &frameComplete);
    void clear();

    ~OMRendererLifeCycle() override;

    auto objType() -> OMRendererObjectType override
    {
        return LifeCycle;
    }

  private:
    struct Entry
    {
        OMRendererBuffer *buffer = nullptr;
        OMRendererTexture *texture = nullptr;
        OMRendererResourceTag tag;
    };

    OMRendererResourceTag currentTag;
    std::vector<Entry> retired;

    log::OMLogger logger;
};
} // namespace openminecraft::renderer::common

#endif
