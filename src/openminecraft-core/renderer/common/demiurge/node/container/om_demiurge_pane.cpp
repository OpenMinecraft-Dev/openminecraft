#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::node::container
{
void OMDemiurgePaneContainer::applyChildClips()
{
    // Only direct children are managed here. Nested panes keep managing their own
    // descendants, and the clips intersect through clipNode resolution.
    auto self = std::static_pointer_cast<OMDemiurgePaneContainer>(shared_from_this());

    for (auto &child : children)
    {
        if (child)
        {
            child->setClipNode(self);
        }
    }
}

auto OMDemiurgePaneContainer::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    applyChildClips();
    OMDemiurgeContainerNode::submit(handler, depth);
}

auto OMDemiurgePaneContainer::update() -> void
{
    applyChildClips();
    OMDemiurgeContainerNode::update();
}
} // namespace openminecraft::renderer::common::demiurge::node::container
