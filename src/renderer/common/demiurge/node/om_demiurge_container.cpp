#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeContainerNode::OMDemiurgeContainerNode() = default;
OMDemiurgeContainerNode::~OMDemiurgeContainerNode() = default;

auto OMDemiurgeContainerNode::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    for (auto c : children)
    {
        c->submit(handler, depth - layerHalfWidth * 2);
    }
}

auto OMDemiurgeContainerNode::remove() -> void
{
    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node
