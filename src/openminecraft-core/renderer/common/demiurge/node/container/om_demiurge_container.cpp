#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::node::container
{
OMDemiurgeContainerNode::OMDemiurgeContainerNode() = default;
OMDemiurgeContainerNode::~OMDemiurgeContainerNode() = default;

auto OMDemiurgeContainerNode::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    for (auto c : children)
    {
        c->submit(handler, c->isLayerOverlay() ? overlayLayer(depth + 1) : depth + 1);
    }
}

auto OMDemiurgeContainerNode::remove() -> void
{
    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::container
