#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <array>
#include <iostream>
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeButton::OMDemiurgeButton(fontproc::OMFontSet *fontset)
{
    this->mountDirect(std::make_shared<OMDemiurgeRectNode>()
                          ->style({
                              {"color", (int)0x23232388},
                              {"radius", glm::vec4(50.0f)},
                              {"flexDirection", OMDemiurgeDirection::Row},
                              {"justifyContent", OMDemiurgeAlign::Center},
                              {"alignItems", OMDemiurgeAlign::Center},
                          })
                          ->mount(std::make_shared<OMDemiurgeTextSdfNode>(fontset)->style({
                              {"text", "OpenMinecraft Demo"},
                              {"textheight", 16},
                              {"color", (int)0xffffffff},
                              {"alignSelf", OMDemiurgeAlign::Center},
                              {"margin", std::array<OMDemiurgeSize, 4>{10_px, 20_px, 10_px, 10_px}},
                          })));
}
OMDemiurgeButton::~OMDemiurgeButton() = default;

auto OMDemiurgeButton::processEvent(float x, float y, OMDemiurgeEventType type, uint8_t ext, void *data)
    -> OMDemiurgeEventResult
{
    if (type == MouseDown)
    {
        std::cout << "button pressed!" << std::endl;
        return Handled;
    }
    else
    {
        return Ignored;
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
