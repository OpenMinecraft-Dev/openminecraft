#include "openminecraft-shell/renderer/om_surface_renderer.hpp"
#include "openminecraft-shell/om_application.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_checkbox.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_badge.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_divider.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_iconbutton.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_link.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_progressbar.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_slider.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_spinner.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_switch.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_container.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/event/om_eventbus_wrap.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <memory>
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_shaderscene.hpp"

using namespace openminecraft::renderer;
using namespace openminecraft;
using namespace openminecraft::renderer::common::demiurge;
using namespace openminecraft::renderer::common;

namespace openminecraft::shell::renderer
{
OMSurfaceRenderer::OMSurfaceRenderer(openminecraft::renderer::OMRenderer *renderer,
                                     openminecraft::renderer::common::event::OMEventBusWrap &bus)
    : OMRendererHandler(renderer), renderer(renderer), offset(0.0), bus(bus)
{
    fontset = std::make_shared<geom::OMFontSet>();

    auto rawfile1 = vfs::fsfetch("/bootassets/openminecraft-renderer/font/MapleMono-NF-Regular.ttf");
    fontset->fontList.push_back(std::make_shared<geom::OMFont>(*rawfile1.get()));

    auto button = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    demoButton = button;
    button->setOnClick([&]() {
        offset.animateTo(-1.05, common::animation::easeInOutCirc<float>, 1.0);

        event::OMEvent e;
        e.type = event::Custom;
        e.custom.flag = 1;
        bus.handle(event::Custom, e);
    });
    bus.append(event::Custom, [&](event::OMEvent &e) {
        if (e.custom.flag == EventMainScreenOpen)
        {
            this->renderer->fetchTask("demiurgeui_compose_surface")->setDisabled(false);
            this->renderer->fetchTask("demiurgeui_bgblur")->setDisabled(false);

            openScreen();
        }

        if (e.custom.flag == EventMainScreenCloseFinish)
        {
            this->renderer->fetchTask("demiurgeui_compose_surface")->setDisabled(true);
            this->renderer->fetchTask("demiurgeui_bgblur")->setDisabled(true);
        }
    });
    button->setBackgroundColor({0.17, 0.17, 0.20});
    auto button2 = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    quitButton = button2;
    button2->setOnClick([&]() {
        event::OMEvent e;
        e.type = event::Custom;
        e.custom.flag = EventQuit;
        bus.handle(event::Custom, e);
    });
    button2->setBackgroundColor({0.17, 0.17, 0.20});

    progressBar = std::make_shared<node::controls::OMDemiurgeProgressBar>();
    progressBar->style({
        {"width", OMDemiurgeSize::pixels(220.0f)},
        {"height", OMDemiurgeSize::pixels(3.0f)},
        {"radius", glm::vec4(3.0f)},
        {"alignSelf", OMDemiurgeAlign::Center},
    });
    progressBar->setTrackColor(glm::vec4(0.106f, 0.106f, 0.141f, 0.35f));
    progressBar->setBarColor(glm::vec4(0.0f, 0.831f, 1.0f, 1.0f));

    checkBox = std::make_shared<node::controls::OMDemiurgeCheckBox>(fontset.get());
    checkBox->style({
        {"label", "Demo Option"},
        {"textheight", 14},
        {"alignSelf", OMDemiurgeAlign::Center},
    });
    checkBox->setBoxColors(glm::vec4(0.1f, 0.1f, 0.12f, 0.65f), glm::vec4(0.0f, 0.55f, 0.75f, 1.0f));
    checkBox->setCheckColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    radioGroup = std::make_shared<node::controls::OMDemiurgeRadioGroup>();
    radioGroup->setDirection(OMDemiurgeDirection::Row);
    radioGroup->setGap(16.0f);
    radioGroup->style("alignSelf", OMDemiurgeAlign::Center);

    radioA = std::make_shared<node::controls::OMDemiurgeRadioBox>(fontset.get());
    radioB = std::make_shared<node::controls::OMDemiurgeRadioBox>(fontset.get());
    radioA->setLabel("Option A");
    radioB->setLabel("Option B");
    radioA->setGroup(radioGroup);
    radioB->setGroup(radioGroup);

    demoSwitch = std::make_shared<node::controls::OMDemiurgeSwitch>();

    demoSlider = std::make_shared<node::controls::OMDemiurgeSlider>();
    demoSlider->style("width", OMDemiurgeSize::pixels(110.0f));
    demoSlider->setValue(0.4f, false);

    demoSpinner = std::make_shared<node::controls::OMDemiurgeSpinner>();
    demoSpinner->setColor(glm::vec4(0.0f, 0.831f, 1.0f, 1.0f));

    demoIconButton = std::make_shared<node::controls::OMDemiurgeIconButton>(24.0f);
    demoIconButton->setIcon("M 3 1 L 12 7 L 3 13 Z", 14.0f);
    demoIconButton->setIconColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    demoBadge = std::make_shared<node::controls::OMDemiurgeBadge>(fontset.get());
    demoBadge->setText("3");

    demoLink = std::make_shared<node::controls::OMDemiurgeLink>(fontset.get());
    demoLink->setText("Docs");

    demoDivider = std::make_shared<node::controls::OMDemiurgeDivider>(true);
    demoDivider->style("height", OMDemiurgeSize::pixels(20.0f));

    auto controlRow = std::make_shared<node::OMDemiurgeContainerNode>();
    controlRow->style({
        {"flexDirection", OMDemiurgeDirection::Row},
        {"alignItems", OMDemiurgeAlign::Center},
        {"flexGap", OMDemiurgeSize::pixels(12.0f)},
        {"alignSelf", OMDemiurgeAlign::Center},
    });
    controlRow->mount(demoSwitch);
    controlRow->mount(demoDivider);
    controlRow->mount(demoSlider);
    controlRow->mount(demoSpinner);
    controlRow->mount(demoIconButton);
    controlRow->mount(demoBadge);
    controlRow->mount(demoLink);

    node = std::make_shared<node::OMDemiurgeRectNode>()
               ->style({
                   {"color", (int)0x2c2c3412},
                   {"flexDirection", Column},
                   {"justifyContent", SpaceBetween},
                   {"width", OMDemiurgeSize::fit()},
                   {"height", OMDemiurgeSize::fit()},
                   {"radius", glm::vec4(5.0f)},
                   {"border", OMDemiurgeEdgeInsets{50, 10, 100, 10}},
               })
               ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                           ->style({
                               {"flexDirection", Column},
                               {"flexGap", 15_px},
                               {"border", OMDemiurgeEdgeInsets{10, 10, 10, 220}},
                           })
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"text", "OpenMinecraft"},
                                           {"textheight", 24},
                                           {"alignSelf", OMDemiurgeAlign::FlexStart},
                                       }))
                           ->mount(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                       ->style({
                                           {"color", (int)0xffffffff},
                                           {"text", fmt::format("Version {}-{}", OM_VERSION, OM_VERSION_CHANNEL)},
                                           {"textheight", 12},
                                           {"border", OMDemiurgeEdgeInsets{0, 60, 0, 0}},
                                       }))
                           ->mount(button->style("animation_speed", 1.0f)->style("label", "Demo Test"))
                           ->mount(button2->style("animation_speed", 1.0f)->style("label", "Quit"))
                           ->mount(progressBar)
                           ->mount(checkBox)
                           ->mount(radioGroup)
                           ->mount(controlRow));

    internal = std::make_shared<OMDemiurgeRendererHandler>(renderer, node);
    internal->name = "surface";
    renderer->registerHandler(internal);

    offset.onAnimateFinish = [&](float v) {
        event::OMEvent e;
        e.type = event::Custom;
        e.custom.flag = v == 0 ? EventMainScreenOpenFinish : EventMainScreenCloseFinish;
        bus.handle(event::Custom, e);
    };
}
OMSurfaceRenderer::~OMSurfaceRenderer()
{
}

void OMSurfaceRenderer::submitTasks()
{
}
void OMSurfaceRenderer::beforeFrame()
{
}
void OMSurfaceRenderer::afterFrame()
{
    node->style("offsetY", OMDemiurgeSize::percent(offset.get()));
}

void OMSurfaceRenderer::openScreen()
{
    offset.animateTo(0.0, common::animation::easeInOutCirc<float>, 1.0);
}
} // namespace openminecraft::shell::renderer