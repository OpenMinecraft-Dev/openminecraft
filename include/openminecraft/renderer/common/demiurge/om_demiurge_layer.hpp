#ifndef OM_DEMIURGE_LAYER_HPP
#define OM_DEMIURGE_LAYER_HPP

#include <cstdint>

namespace openminecraft::renderer::common::demiurge
{
// A layer is a uint32_t:
//   [31]     overlay group (rendered after the normal group)
//   [30:16]  15-bit z-order (higher is closer to the viewer)
//   [15:0]   actual layer index
//
// The z-order is converted to a 0..1 clip-space depth by the UI vertex
// shaders. The actual layer index still selects the draw bucket/order.
constexpr uint32_t layerGroupBit = 0x80000000u;
constexpr uint32_t layerIndexMask = 0x0000FFFFu;
constexpr uint32_t layerZOrderShift = 16u;
constexpr uint32_t layerZOrderMask = 0x7FFF0000u;
constexpr uint32_t layerZOrderMax = 0x7FFFu;
constexpr uint32_t layerKeyMask = layerGroupBit | layerIndexMask;

constexpr auto isOverlayLayer(uint32_t layer) -> bool
{
    return (layer & layerGroupBit) != 0;
}

constexpr auto layerIndex(uint32_t layer) -> uint32_t
{
    return layer & layerIndexMask;
}

constexpr auto layerZOrder(uint32_t layer) -> uint32_t
{
    return (layer & layerZOrderMask) >> layerZOrderShift;
}

constexpr auto withLayerZOrder(uint32_t layer, uint32_t zorder) -> uint32_t
{
    return (layer & ~layerZOrderMask) | ((zorder & layerZOrderMax) << layerZOrderShift);
}

constexpr auto overlayLayer(uint32_t layer) -> uint32_t
{
    return layer | layerGroupBit;
}

// Identifies the actual draw bucket while ignoring z-order.
constexpr auto layerKey(uint32_t layer) -> uint32_t
{
    return layer & layerKeyMask;
}

constexpr auto sameLayer(uint32_t a, uint32_t b) -> bool
{
    return layerKey(a) == layerKey(b);
}
} // namespace openminecraft::renderer::common::demiurge

#endif
