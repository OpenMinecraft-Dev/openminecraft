#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::node::container
{
OMDemiurgePane::OMDemiurgePane() = default;
OMDemiurgePane::~OMDemiurgePane() = default;

auto OMDemiurgePane::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    for (auto c : children)
    {
        c->submit(handler, c->isLayerOverlay() ? overlayLayer(depth + 1) : depth + 1);
    }
}

auto OMDemiurgePane::remove() -> void
{
    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::container
