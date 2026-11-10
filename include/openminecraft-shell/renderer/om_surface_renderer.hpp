#ifndef OM_SURFACE_RENDERER_HPP
#define OM_SURFACE_RENDERER_HPP

#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/om_renderer_handler.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_checkbox.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_radiobox.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_badge.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_divider.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_iconbutton.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_textedit.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_tabs.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_scrollbar.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_scrollpane.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_textinput.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_link.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_progressbar.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_slider.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_spinner.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_switch.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"
namespace openminecraft::shell::renderer
{
class OMSurfaceRenderer : public openminecraft::renderer::common::OMRendererHandler
{
  public:
    OMSurfaceRenderer(openminecraft::renderer::OMRenderer *renderer,
                      openminecraft::renderer::common::event::OMEventBusWrap &);
    virtual ~OMSurfaceRenderer() override;

    void submitTasks() override;
    void beforeFrame() override;
    void afterFrame() override;

    openminecraft::renderer::OMRenderer *renderer;
    std::shared_ptr<openminecraft::renderer::common::demiurge::OMDemiurgeNode> node;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeProgressBar> progressBar;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeCheckBox> checkBox;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeButton> demoButton, quitButton;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeRadioGroup> radioGroup;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeRadioBox> radioA, radioB;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeSwitch> demoSwitch;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeSlider> demoSlider;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeSpinner> demoSpinner;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeIconButton> demoIconButton;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeBadge> demoBadge;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeLink> demoLink;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeDivider> demoDivider;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeTextInput> demoInput;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeTextEdit> demoTextEdit;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeTabMultiPage> demoTabs;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeScrollBar> demoScrollBar;
    std::shared_ptr<openminecraft::renderer::common::demiurge::node::controls::OMDemiurgeScrollPane> demoScrollPane;
    std::shared_ptr<openminecraft::renderer::common::demiurge::OMDemiurgeRendererHandler> internal;
    openminecraft::renderer::common::animation::OMAnimationValue<float> offset;

    std::shared_ptr<openminecraft::geom::OMFontSet> fontset;

    openminecraft::renderer::common::event::OMEventBusWrap &bus;
    void openScreen();
};
} // namespace openminecraft::shell::renderer

#endif