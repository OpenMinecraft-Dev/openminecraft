#ifndef OM_DEMIURGE_ELEMENT_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_CHANNEL_HPP

#include "openminecraft/renderer/common/demiurge/om_demiurge_layer.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_task.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>
namespace openminecraft::renderer::common::demiurge::element
{
template <typename T> auto objectGetDepth(T &obj) -> uint32_t;
template <typename T> auto objectSetDepth(T &obj, uint32_t) -> void;

class OMDemiurgeAbstractChannel
{
  public:
    OMDemiurgeAbstractChannel() = default;
    ~OMDemiurgeAbstractChannel() = default;

    virtual void init(const std::string &uniformTag, const std::string &targetTag) = 0;
    virtual void submitTask(OMRendererTask *task, uint32_t layer) = 0;
    virtual void update() = 0;
    virtual void destroy() = 0;
};
template <typename T> class OMDemiurgeChannel : public OMDemiurgeAbstractChannel
{
  public:
    OMDemiurgeChannel() = default;
    ~OMDemiurgeChannel() = default;

    void init(const std::string &uniformTag, const std::string &targetTag) override = 0;
    void submitTask(OMRendererTask *task, uint32_t layer) override = 0;
    void update() override = 0;
    void destroy() override = 0;

    inline auto request(uint32_t depth) -> int
    {
        if (!removed.empty())
        {
            for (auto it = removed.begin(); it != removed.end(); ++it)
            {
                if (sameLayer(objectGetDepth(objects[*it]), depth))
                {
                    auto i = *it;
                    removed.erase(it);
                    active[i] = true;
                    updateMaxLayer(depth);
                    return i;
                }
            }
        }
        objects.emplace_back(T{});
        dirty.resize(objects.size());
        clipBounds.resize(objects.size());
        active.resize(objects.size(), true);
        hasDirty = true;
        updateMaxLayer(depth);
        return objects.size() - 1;
    }

    inline auto temporary(int i) -> T *
    {
        dirty[i] = true;
        hasDirty = true;
        return &objects[i];
    }

    inline auto setClip(int i, const std::optional<OMDemiurgeRect> &rect) -> bool
    {
        if (i < 0 || static_cast<size_t>(i) >= clipBounds.size())
        {
            return false;
        }

        const auto &current = clipBounds[i];

        if (current.has_value() == rect.has_value())
        {
            if (!current.has_value() ||
                (current->x == rect->x && current->y == rect->y && current->width == rect->width &&
                 current->height == rect->height))
            {
                return false;
            }
        }

        clipBounds[i] = rect;
        return true;
    }

    inline auto fetchClip(int i) const -> std::optional<OMDemiurgeRect>
    {
        if (i >= 0 && static_cast<size_t>(i) < clipBounds.size())
        {
            return clipBounds[i];
        }

        return std::nullopt;
    }

    inline auto solve() -> void
    {
        hasDirty = false;
        dirty.assign(dirty.size(), false);
    }
    inline auto remove(int i) -> void
    {
        auto depth = objectGetDepth<T>(objects[i]);
        onRemove(i);
        objects[i] = T{};
        objectSetDepth(objects[i], depth);
        dirty[i] = true;
        hasDirty = true;
        active[i] = false;
        removed.emplace_back(i);

        const uint32_t maxLayer = isOverlayLayer(depth) ? overlayLayer(maxLayerOverlayValue) : maxLayerNormalValue;
        if (sameLayer(depth, maxLayer))
        {
            recomputeMaxLayers();
        }
    }

    virtual auto onRemove(int i) -> void
    {
    }

    inline auto dirtyIter(std::function<void(int begin, int length)> f) -> void
    {
        if (hasDirty)
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

    inline auto maxLayer(bool overlay = false) -> uint32_t
    {
        return overlay ? maxLayerOverlayValue : maxLayerNormalValue;
    }

    inline auto hasLayerGroup(bool overlay = false) -> bool
    {
        return overlay ? hasOverlayObjects : hasNormalObjects;
    }

    inline auto bufferSize() -> int
    {
        // Grow the instance buffer by a larger factor than the object count so
        // adding a few objects does not force a GPU buffer reallocation every
        // time. The unused tail stays allocated for later frames.
        constexpr size_t growthFactor = 4;

        if (objects.size() > bufferCapacity)
        {
            size_t next = std::max<size_t>(bufferCapacity, 8);

            while (next < objects.size())
            {
                next *= growthFactor;
            }

            bufferCapacity = next;
        }

        return sizeof(T) * bufferCapacity;
    }

  protected:
    std::vector<int> removed;
    std::vector<T> objects;
    std::vector<bool> dirty;
    std::vector<std::optional<OMDemiurgeRect>> clipBounds;
    bool hasDirty = true;
    int lastCount = -1;
    size_t bufferCapacity = 0;
    std::vector<bool> active;
    uint32_t maxLayerNormalValue = 0;
    uint32_t maxLayerOverlayValue = 0;
    bool hasNormalObjects = false;
    bool hasOverlayObjects = false;

    inline auto updateMaxLayer(uint32_t depth) -> void
    {
        if (isOverlayLayer(depth))
        {
            maxLayerOverlayValue = std::max(maxLayerOverlayValue, layerIndex(depth));
            hasOverlayObjects = true;
        }
        else
        {
            maxLayerNormalValue = std::max(maxLayerNormalValue, layerIndex(depth));
            hasNormalObjects = true;
        }
    }

    inline auto recomputeMaxLayers() -> void
    {
        maxLayerNormalValue = 0;
        maxLayerOverlayValue = 0;
        hasNormalObjects = false;
        hasOverlayObjects = false;

        for (size_t j = 0; j < objects.size(); ++j)
        {
            if (j < active.size() && active[j])
            {
                updateMaxLayer(objectGetDepth<T>(objects[j]));
            }
        }
    }
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
