#ifndef OM_DEMIURGE_LAYER_HPP
#define OM_DEMIURGE_LAYER_HPP

#include <cstdint>

namespace openminecraft::renderer::common::demiurge
{
// A layer is a uint32_t. The highest bit selects a separate layer group that is
// rendered after all normal layers (used by dialogs/overlays). The remaining
// bits are the layer index inside that group.
constexpr uint32_t layerGroupBit = 0x80000000u;
constexpr uint32_t layerIndexMask = ~layerGroupBit;

constexpr auto isOverlayLayer(uint32_t layer) -> bool
{
    return (layer & layerGroupBit) != 0;
}

constexpr auto layerIndex(uint32_t layer) -> uint32_t
{
    return layer & layerIndexMask;
}

constexpr auto overlayLayer(uint32_t layer) -> uint32_t
{
    return layer | layerGroupBit;
}
} // namespace openminecraft::renderer::common::demiurge

#endif
