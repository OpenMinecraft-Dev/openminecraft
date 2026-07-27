#ifndef OM_DEMIURGE_ELEMENT_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_CHANNEL_HPP

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include <functional>
#include <vector>
namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeAbstractChannel
{
  public:
    OMDemiurgeAbstractChannel() = default;
    ~OMDemiurgeAbstractChannel() = default;

    virtual void init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) = 0;
    virtual void submitTask(OMRendererTask *task, float upper, float lower,
                            OMDemiurgeAbstractChannel *&currentChannel) = 0;
    virtual void update() = 0;
    virtual void destroy() = 0;
};
template <typename T> class OMDemiurgeChannel : public OMDemiurgeAbstractChannel
{
  public:
    OMDemiurgeChannel() = default;
    ~OMDemiurgeChannel() = default;

    void init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) override = 0;
    void submitTask(OMRendererTask *task, float upper, float lower,
                    OMDemiurgeAbstractChannel *&currentChannel) override = 0;
    void update() override = 0;
    void destroy() override = 0;

    inline auto request() -> int
    {
        objects.emplace_back(T{});
        dirty.resize(objects.size());
        return objects.size() - 1;
    }

    inline auto temporary(int i) -> T *
    {
        dirty[i] = true;
        return &objects[i];
    }

    inline auto solve() -> void
    {
        dirty.assign(dirty.size(), false);
    }
    inline auto remove(int i) -> void
    {
        objects[i] = T{};
        dirty[i] = true;
    }

    inline auto dirtyIter(std::function<void(int begin, int length)> f) -> void
    {
        if (std::find(dirty.begin(), dirty.end(), true) != dirty.end())
        {
            bool in_dirty = false;
            int start = 0;
            for (int i = 0; i <= dirty.size(); ++i)
            {
                bool is_dirty = (i < dirty.size()) && dirty[i];
                if (!in_dirty && is_dirty)
                {
                    start = i;
                    in_dirty = true;
                }
                else if (in_dirty && !is_dirty)
                {
                    f(start, i - start);
                    in_dirty = false;
                }
            }
            solve();
        }
    }

    inline auto bufferSize() -> int
    {
        return sizeof(T) * objects.size();
    }

  protected:
    std::vector<T> objects;
    std::vector<bool> dirty;
    int lastCount = -1;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
