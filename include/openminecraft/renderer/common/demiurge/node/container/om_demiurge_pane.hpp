#ifndef OM_DEMIURGE_PANE_HPP
#define OM_DEMIURGE_PANE_HPP

#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"

namespace openminecraft::renderer::common::demiurge::node::container
{
// Container that clips every direct child to its own bounds.
//
// Nested panes are handled hierarchically: a pane assigns itself as the clip node
// of its direct children only. When a child is another pane, that pane keeps
// managing its own descendants, and the clips intersect through the normal
// clipNode resolution.
class OMDemiurgePaneContainer : public OMDemiurgeContainerNode
{
  public:
    OMDemiurgePaneContainer() = default;
    ~OMDemiurgePaneContainer() override = default;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto update() -> void override;

    auto isClipBoundary() const -> bool override
    {
        return true;
    }

  private:
    auto applyChildClips() -> void;
    auto applyClipsRecursive(const std::shared_ptr<OMDemiurgeNode> &node,
                             const std::shared_ptr<OMDemiurgePaneContainer> &self) -> void;
};
} // namespace openminecraft::renderer::common::demiurge::node::container

namespace openminecraft::renderer::common::demiurge::node
{
using container::OMDemiurgePaneContainer;
} // namespace openminecraft::renderer::common::demiurge::node

#endif
