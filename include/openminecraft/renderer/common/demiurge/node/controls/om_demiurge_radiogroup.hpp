#ifndef OM_DEMIURGE_RADIOGROUP_HPP
#define OM_DEMIURGE_RADIOGROUP_HPP

#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include <memory>
#include <vector>

namespace openminecraft::renderer::common::demiurge::node::controls
{
class OMDemiurgeRadioBox;

// Radio group: lays the radios out (row/column) and owns the selection policy.
class OMDemiurgeRadioGroup : public OMDemiurgePane
{
  public:
    OMDemiurgeRadioGroup();

    void add(const std::shared_ptr<OMDemiurgeRadioBox> &box);

    // Single-select groups clear the previous member; multi-select groups keep them.
    void select(const std::shared_ptr<OMDemiurgeRadioBox> &box);

    // Click policy: single selects, multi toggles.
    void click(const std::shared_ptr<OMDemiurgeRadioBox> &box);

    void setDirection(OMDemiurgeDirection direction);
    void setGap(float pixels);
    void setMultiSelect(bool value);
    auto isMultiSelect() const -> bool
    {
        return multiSelect;
    }

  private:
    std::vector<std::weak_ptr<OMDemiurgeRadioBox>> members;
    bool multiSelect = false;
};

} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
