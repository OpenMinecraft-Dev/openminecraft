#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::node::container
{
void OMDemiurgePaneContainer::applyClipsRecursive(const std::shared_ptr<OMDemiurgeNode> &node,
                                                 const std::shared_ptr<OMDemiurgePaneContainer> &self)
{
    for (auto &child : node->fetchChildren())
    {
        if (!child)
        {
            continue;
        }

        child->setClipNode(self);

        // A nested pane owns its own descendants; do not override them here.
        if (child->isClipBoundary())
        {
            continue;
        }

        applyClipsRecursive(child, self);
    }
}

void OMDemiurgePaneContainer::applyChildClips()
{
    // Constrain the whole subtree (not only direct children). Nested panes are
    // left alone below their own boundary; clips still intersect via clipNode.
    auto self = std::static_pointer_cast<OMDemiurgePaneContainer>(shared_from_this());
    applyClipsRecursive(self, self);
}

auto OMDemiurgePaneContainer::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
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
