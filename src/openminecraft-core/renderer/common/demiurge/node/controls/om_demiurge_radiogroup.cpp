#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_radiogroup.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_radiobox.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"

#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_radiobox.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>
#include <array>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
static constexpr float RadioSize = 16.0f;
static constexpr float DotSize = 7.0f;

OMDemiurgeRadioGroup::OMDemiurgeRadioGroup()
{
    stylesStorage.put("flexDirection", OMDemiurgeDirection::Column);
    stylesStorage.put("alignItems", OMDemiurgeAlign::FlexStart);
    stylesStorage.put("flexGap", OMDemiurgeSize::pixels(6.0f));
    stylesStorage.put("width", OMDemiurgeSize::fit());
    stylesStorage.put("height", OMDemiurgeSize::fit());
}

void OMDemiurgeRadioGroup::add(const std::shared_ptr<OMDemiurgeRadioBox> &box)
{
    if (!box)
    {
        return;
    }

    for (auto &m : members)
    {
        if (auto current = m.lock(); current == box)
        {
            return;
        }
    }

    members.push_back(box);
    this->mount(box);
}

void OMDemiurgeRadioGroup::select(const std::shared_ptr<OMDemiurgeRadioBox> &box)
{
    for (auto it = members.begin(); it != members.end();)
    {
        auto current = it->lock();

        if (!current)
        {
            it = members.erase(it);
            continue;
        }

        if (current != box && !multiSelect)
        {
            current->applySelection(false, true, false);
        }

        ++it;
    }

    if (box)
    {
        box->applySelection(true, true, true);
    }
}

void OMDemiurgeRadioGroup::click(const std::shared_ptr<OMDemiurgeRadioBox> &box)
{
    if (!box)
    {
        return;
    }

    if (multiSelect)
    {
        box->applySelection(!box->isSelected(), true, true);
    }
    else
    {
        select(box);
    }
}

void OMDemiurgeRadioGroup::setDirection(OMDemiurgeDirection direction)
{
    stylesStorage.put("flexDirection", direction);
    stylesStorage.put("alignItems", direction == OMDemiurgeDirection::Row ? OMDemiurgeAlign::Center
                                                                          : OMDemiurgeAlign::FlexStart);
}

void OMDemiurgeRadioGroup::setGap(float pixels)
{
    stylesStorage.put("flexGap", OMDemiurgeSize::pixels(pixels));
}

void OMDemiurgeRadioGroup::setMultiSelect(bool value)
{
    multiSelect = value;
}

} // namespace openminecraft::renderer::common::demiurge::node::controls
