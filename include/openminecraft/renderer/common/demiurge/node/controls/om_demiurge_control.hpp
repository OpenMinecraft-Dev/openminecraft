#ifndef OM_DEMIURGE_CONTROL_HPP
#define OM_DEMIURGE_CONTROL_HPP

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Shared API surface for interactive controls. Controls keep their own concrete
// base (rect/container), so this is a lightweight interface rather than a node.
class OMDemiurgeControl
{
  public:
    virtual ~OMDemiurgeControl() = default;

    virtual void setDisabled(bool value, bool animate = true) = 0;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
