#include "openminecraft-shell/renderer/om_surface_renderer.hpp"
#include "openminecraft-shell/om_application.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_checkbox.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_badge.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_divider.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_iconbutton.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_textedit.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_dropdown.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_tabs.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_textinput.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_link.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_progressbar.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_slider.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_spinner.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_switch.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_container.hpp"
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
    button->setBackgroundColor({0.17f, 0.17f, 0.20f, 1.0f});
    auto button2 = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    quitButton = button2;
    button2->setOnClick([&]() {
        event::OMEvent e;
        e.type = event::Custom;
        e.custom.flag = EventQuit;
        bus.handle(event::Custom, e);
    });
    button2->setBackgroundColor({0.17f, 0.17f, 0.20f, 1.0f});

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
    demoSlider->style("width", OMDemiurgeSize::pixels(120.0f));
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

    demoInput = std::make_shared<node::controls::OMDemiurgeTextInput>(fontset.get());
    demoInput->setPlaceholder("Type here...");
    demoInput->style("width", OMDemiurgeSize::pixels(220.0f));

    demoTextEdit = std::make_shared<node::controls::OMDemiurgeTextEdit>(fontset.get());
    demoTextEdit->setPlaceholder("Multi-line text...");
    demoTextEdit->style({{"width", OMDemiurgeSize::pixels(260.0f)}, {"height", OMDemiurgeSize::pixels(36.0f)}});

    progressBar->setProgress(0.62f);

    auto progressBarIndeterminate = std::make_shared<node::controls::OMDemiurgeProgressBar>();
    progressBarIndeterminate->style({
        {"width", OMDemiurgeSize::fill()},
        {"height", OMDemiurgeSize::pixels(3.0f)},
        {"radius", glm::vec4(3.0f)},
    });
    progressBarIndeterminate->setTrackColor(glm::vec4(0.106f, 0.106f, 0.141f, 0.35f));
    progressBarIndeterminate->setBarColor(glm::vec4(0.0f, 0.831f, 1.0f, 1.0f));
    progressBarIndeterminate->setIndeterminate(true);

    auto progressContent = std::make_shared<node::OMDemiurgeContainerNode>();
    progressContent->style({{"flexDirection", OMDemiurgeDirection::Column},
                            {"flexGap", OMDemiurgeSize::pixels(4.0f)},
                            {"width", OMDemiurgeSize::fill()}});
    progressContent->mountDirect(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                     ->style({{"color", (int)0xffffff8c},
                                              {"text", "Determinate"},
                                              {"textheight", 11},
                                              {"alignSelf", OMDemiurgeAlign::FlexStart}}));
    progressBar->style("width", OMDemiurgeSize::fill());
    progressContent->mountDirect(progressBar);
    progressContent->mountDirect(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                     ->style({{"color", (int)0xffffff8c},
                                              {"text", "Indeterminate"},
                                              {"textheight", 11},
                                              {"alignSelf", OMDemiurgeAlign::FlexStart},
                                              {"border", OMDemiurgeEdgeInsets{8, 0, 0, 0}}}));
    progressContent->mountDirect(progressBarIndeterminate);

    auto makeDropdown = [&](const std::string &title,
                            const std::shared_ptr<OMDemiurgeNode> &content) -> std::shared_ptr<node::controls::OMDemiurgeDropDownPane> {
        auto contentWrap = std::make_shared<node::OMDemiurgeContainerNode>();
        contentWrap->style({{"flexDirection", OMDemiurgeDirection::Column},
                            {"alignItems", OMDemiurgeAlign::Center},
                            {"flexGap", OMDemiurgeSize::pixels(6.0f)},
                            {"width", OMDemiurgeSize::fill()},
                            {"padding", OMDemiurgeEdgeInsets{6, 6, 6, 6}}});
        contentWrap->mountDirect(content);

        auto pane = std::make_shared<node::controls::OMDemiurgeDropDownPane>(fontset.get());
        pane->setTitle(title);
        pane->style("width", OMDemiurgeSize::fill());
        pane->setHeaderHeight(30.0f);
        pane->setContentNode(contentWrap);
        return pane;
    };

    auto galleryColumn1 = std::make_shared<node::OMDemiurgeContainerNode>();
    galleryColumn1->style({{"flexDirection", OMDemiurgeDirection::Column},
                           {"flexGap", OMDemiurgeSize::pixels(8.0f)},
                           {"flexGrow", 1.0f},
                           {"flexBasis", OMDemiurgeSize::pixels(0.0f)}});
    demoDropDown = makeDropdown("ProgressBar", progressContent);
    galleryColumn1->mountDirect(demoDropDown);
    galleryColumn1->mountDirect(makeDropdown("CheckBox", checkBox));
    galleryColumn1->mountDirect(makeDropdown("RadioGroup", radioGroup));
    galleryColumn1->mountDirect(makeDropdown("Switch", demoSwitch));
    galleryColumn1->mountDirect(makeDropdown("Divider", demoDivider));
    galleryColumn1->mountDirect(makeDropdown("Slider", demoSlider));

    auto galleryColumn2 = std::make_shared<node::OMDemiurgeContainerNode>();
    galleryColumn2->style({{"flexDirection", OMDemiurgeDirection::Column},
                           {"flexGap", OMDemiurgeSize::pixels(8.0f)},
                           {"flexGrow", 1.0f},
                           {"flexBasis", OMDemiurgeSize::pixels(0.0f)}});
    galleryColumn2->mountDirect(makeDropdown("Spinner", demoSpinner));
    galleryColumn2->mountDirect(makeDropdown("IconButton", demoIconButton));
    galleryColumn2->mountDirect(makeDropdown("Badge", demoBadge));
    galleryColumn2->mountDirect(makeDropdown("Link", demoLink));
    demoInput->style("width", OMDemiurgeSize::fill());
    galleryColumn2->mountDirect(makeDropdown("TextInput", demoInput));
    demoTextEdit->style("width", OMDemiurgeSize::fill());
    galleryColumn2->mountDirect(makeDropdown("TextEdit", demoTextEdit));

    auto galleryGrid = std::make_shared<node::OMDemiurgeContainerNode>();
    galleryGrid->style({{"flexDirection", OMDemiurgeDirection::Row},
                        {"alignItems", OMDemiurgeAlign::FlexStart},
                        {"flexGap", OMDemiurgeSize::pixels(12.0f)},
                        {"width", OMDemiurgeSize::fill()}});
    galleryGrid->mountDirect(galleryColumn1);
    galleryGrid->mountDirect(galleryColumn2);

    auto pageMain = std::make_shared<node::OMDemiurgeContainerNode>();
    pageMain->style(
        {{"flexDirection", Column}, {"flexGap", OMDemiurgeSize::pixels(15.0f)}, {"width", OMDemiurgeSize::fill()}});
    pageMain->mountDirect(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                              ->style({{"color", (int)0xffffffff},
                                       {"text", "OpenMinecraft"},
                                       {"textheight", 24},
                                       {"alignSelf", OMDemiurgeAlign::FlexStart}}));
    pageMain->mountDirect(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                              ->style({{"color", (int)0xffffffff},
                                       {"text", fmt::format("Version {}-{}", OM_VERSION, OM_VERSION_CHANNEL)},
                                       {"textheight", 12},
                                       {"border", OMDemiurgeEdgeInsets{0, 60, 0, 0}}}));
    pageMain->mountDirect(button->style("animation_speed", 1.0f)->style("label", "Demo Test"));
    pageMain->mountDirect(button2->style("animation_speed", 1.0f)->style("label", "Quit"));

    auto pageGallery = std::make_shared<node::OMDemiurgeContainerNode>();
    pageGallery->style({{"flexDirection", Column},
                        {"alignItems", OMDemiurgeAlign::Stretch},
                        {"flexGap", OMDemiurgeSize::pixels(12.0f)},
                        {"width", OMDemiurgeSize::fill()}});
    pageGallery->mountDirect(std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get())
                                 ->style({{"color", (int)0xffffffff},
                                          {"text", "Control Gallery"},
                                          {"textheight", 18},
                                          {"alignSelf", OMDemiurgeAlign::FlexStart}}));
    pageGallery->mountDirect(galleryGrid);

    demoTabs = std::make_shared<node::controls::OMDemiurgeTabMultiPage>(fontset.get());
    demoTabs->style("width", OMDemiurgeSize::fill());
    demoTabs->style("height", OMDemiurgeSize::fill());
    demoTabs->setPageFill(true);
    demoTabs->addPage("Main", pageMain);
    demoTabs->addPage("Gallery", pageGallery);

    node = std::make_shared<node::OMDemiurgeRectNode>()
               ->style({
                   {"color", (int)0x2c2c3412},
                   {"flexDirection", Column},
                   {"justifyContent", SpaceBetween},
                   {"width", OMDemiurgeSize::fill()},
                   {"height", OMDemiurgeSize::fill()},
                   {"radius", glm::vec4(5.0f)},
                   {"border", OMDemiurgeEdgeInsets{10, 10, 10, 10}},
               })
               ->mount(std::make_shared<node::OMDemiurgeContainerNode>()
                           ->style({
                               {"flexDirection", Column},
                               {"width", OMDemiurgeSize::fill()},
                               {"height", OMDemiurgeSize::fill()},
                               {"flexGap", 15_px},
                               {"border", OMDemiurgeEdgeInsets{10, 10, 10, 10}},
                           })
                           ->mount(demoTabs));

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