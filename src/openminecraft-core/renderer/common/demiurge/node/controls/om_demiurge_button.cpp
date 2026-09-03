#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeButton::OMDemiurgeButton(fontproc::OMFontSet *fontset)
{
    this->mountDirect(std::make_shared<OMDemiurgeRectNode>()
                          ->style({
                              {"color", (int)0x000000ff},
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
                          })));
}
OMDemiurgeButton::~OMDemiurgeButton()
{
}
} // namespace openminecraft::renderer::common::demiurge::node::controls