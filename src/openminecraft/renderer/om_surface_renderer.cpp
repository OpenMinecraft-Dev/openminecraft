#include "openminecraft-shell/renderer/om_surface_renderer.hpp"
#include "openminecraft-shell/om_application.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_checkbox.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_radiogroup.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_badge.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_divider.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_iconbutton.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_textedit.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_dropdown.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_tabs.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_scrollbar.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_scrollpane.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_textinput.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_link.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_progressbar.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_slider.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_spinner.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_switch.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_dialog.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_label.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_tooltip.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_multiline_label.hpp"
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
    node::controls::OMDemiurgeTooltip::instance().setFontSet(fontset.get());
    dialog = std::make_shared<node::controls::OMDemiurgeDialog>(fontset.get());
    dialog2 = std::make_shared<node::controls::OMDemiurgeDialog>(fontset.get());

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

        if (e.custom.flag == EventChunkLoadStatus && chunkStatusBar)
        {
            chunkStatusBar->setProgress(e.custom.value);
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

    checkBox = std::make_shared<node::controls::OMDemiurgeCheckBox>(fontset.get());
    checkBox->style({
        {"label", "Demo Option"},
        {"textHeight", 14},
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

    demoInput = std::make_shared<node::controls::OMDemiurgeTextInput>(fontset.get());
    demoInput->setPlaceholder("Type here...");
    demoInput->style("width", OMDemiurgeSize::pixels(220.0f));

    demoTextEdit = std::make_shared<node::controls::OMDemiurgeTextEdit>(fontset.get());
    demoTextEdit->setPlaceholder("Multi-line text...");
    demoTextEdit->style({{"width", OMDemiurgeSize::pixels(260.0f)}, {"height", OMDemiurgeSize::pixels(120.0f)}});

    demoProgressBar = std::make_shared<node::controls::OMDemiurgeProgressBar>();
    demoProgressBar->style({
        {"width", OMDemiurgeSize::fill()},
        {"height", OMDemiurgeSize::pixels(3.0f)},
        {"radius", glm::vec4(3.0f)},
    });
    demoProgressBar->setTrackColor(glm::vec4(0.106f, 0.106f, 0.141f, 0.35f));
    demoProgressBar->setBarColor(glm::vec4(0.0f, 0.831f, 1.0f, 1.0f));
    demoProgressBar->setIndeterminate(true);

    demoProgError = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    demoProgError->style("label", "Error");
    demoProgError->setOnClick([&]() {
        demoProgressBar->setError(true);
        demoProgressBar->setIndeterminate(false);
        demoProgressBar->setProgress(1.0f);
    });
    demoProgIndetermined = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    demoProgIndetermined->style("label", "Indetermined");
    demoProgIndetermined->setOnClick([&]() {
        demoProgressBar->setError(false);
        demoProgressBar->setIndeterminate(true);
    });
    demoProgNorm = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    demoProgNorm->style("label", "Norm");
    demoProgNorm->setOnClick([&]() {
        demoProgressBar->setError(false);
        demoProgressBar->setIndeterminate(false);
        demoProgressBar->setProgress(0.6f);
    });

    auto progressContent = std::make_shared<node::OMDemiurgePane>();
    progressContent->style({{"flexDirection", OMDemiurgeDirection::Column},
                            {"flexGap", OMDemiurgeSize::pixels(4.0f)},
                            {"width", OMDemiurgeSize::fill()}});
    progressContent->mountDirect(demoProgressBar);

    auto progressButtons = std::make_shared<node::OMDemiurgePane>();
    progressButtons->style({{"flexDirection", OMDemiurgeDirection::Row},
                            {"alignItems", OMDemiurgeAlign::Center},
                            {"flexGap", OMDemiurgeSize::pixels(8.0f)},
                            {"width", OMDemiurgeSize::fill()}});
    auto styleProgressButton = [](const std::shared_ptr<node::controls::OMDemiurgeButton> &button) {
        button->style("width", OMDemiurgeSize::pixels(84.0f));
        button->style("height", OMDemiurgeSize::pixels(22.0f));
        button->findChildById("text")->style(
            {{"textHeight", 11}, {"margin", std::array<OMDemiurgeSize, 4>{2_px, 2_px, 6_px, 6_px}}});
        button->findChildById("background")->style("radius", glm::vec4(4.0f));
        button->setBackgroundColor({0.17f, 0.17f, 0.20f, 1.0f});
    };
    styleProgressButton(demoProgError);
    styleProgressButton(demoProgIndetermined);
    styleProgressButton(demoProgNorm);
    progressButtons->mountDirect(demoProgError);
    progressButtons->mountDirect(demoProgIndetermined);
    progressButtons->mountDirect(demoProgNorm);
    progressContent->mountDirect(progressButtons);

    auto scrollBarVertical = std::make_shared<node::controls::OMDemiurgeScrollBar>();
    demoScrollBar = scrollBarVertical;
    scrollBarVertical->style("width", OMDemiurgeSize::pixels(4.0f));
    scrollBarVertical->style("height", OMDemiurgeSize::pixels(80.0f));
    scrollBarVertical->setMetrics(80.0f, 320.0f);
    auto scrollLabelVertical = std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get());
    scrollLabelVertical->style({{"color", (int)0xffffff8c}, {"text", "V: 0.0"}, {"textHeight", 11}});
    scrollBarVertical->setOnScroll(
        [scrollLabelVertical](float value) { scrollLabelVertical->style("text", fmt::format("V: {:.1f}", value)); });
    auto scrollRowVertical = std::make_shared<node::OMDemiurgePane>();
    scrollRowVertical->style({{"flexDirection", OMDemiurgeDirection::Row},
                              {"alignItems", OMDemiurgeAlign::Center},
                              {"flexGap", OMDemiurgeSize::pixels(12.0f)},
                              {"width", OMDemiurgeSize::fill()}});
    scrollRowVertical->mountDirect(scrollBarVertical);
    scrollRowVertical->mountDirect(scrollLabelVertical);

    auto scrollBarHorizontal = std::make_shared<node::controls::OMDemiurgeScrollBar>();
    scrollBarHorizontal->setOrientation(node::controls::OMDemiurgeScrollBarOrientation::Horizontal);
    scrollBarHorizontal->style("width", OMDemiurgeSize::pixels(220.0f));
    scrollBarHorizontal->style("height", OMDemiurgeSize::pixels(4.0f));
    scrollBarHorizontal->setMetrics(120.0f, 400.0f);
    auto scrollLabelHorizontal = std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get());
    scrollLabelHorizontal->style({{"color", (int)0xffffff8c}, {"text", "H: 0.0"}, {"textHeight", 11}});
    scrollBarHorizontal->setOnScroll([scrollLabelHorizontal](float value) {
        scrollLabelHorizontal->style("text", fmt::format("H: {:.1f}", value));
    });
    auto scrollRowHorizontal = std::make_shared<node::OMDemiurgePane>();
    scrollRowHorizontal->style({{"flexDirection", OMDemiurgeDirection::Row},
                                {"alignItems", OMDemiurgeAlign::Center},
                                {"flexGap", OMDemiurgeSize::pixels(12.0f)},
                                {"width", OMDemiurgeSize::fill()}});
    scrollRowHorizontal->mountDirect(scrollBarHorizontal);
    scrollRowHorizontal->mountDirect(scrollLabelHorizontal);

    auto scrollContent = std::make_shared<node::OMDemiurgePane>();
    scrollContent->style({{"flexDirection", OMDemiurgeDirection::Column},
                          {"flexGap", OMDemiurgeSize::pixels(10.0f)},
                          {"width", OMDemiurgeSize::fill()}});
    scrollContent->mountDirect(scrollRowVertical);
    scrollContent->mountDirect(scrollRowHorizontal);

    auto galleryList = std::make_shared<node::OMDemiurgePane>();
    galleryList->style({{"flexDirection", OMDemiurgeDirection::Column}, {"width", OMDemiurgeSize::fill()}});

    auto addGallerySection = [&](const std::string &title, const std::string &description,
                                 const std::shared_ptr<OMDemiurgeNode> &body) {
        auto section = std::make_shared<node::OMDemiurgePane>();
        section->style({{"flexDirection", OMDemiurgeDirection::Column},
                        {"flexGap", OMDemiurgeSize::pixels(6.0f)},
                        {"width", OMDemiurgeSize::fill()},
                        {"padding", OMDemiurgeEdgeInsets{12, 12, 12, 12}}});
        section->mountDirect(std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get())
                                 ->style({{"color", (int)0xffffffff},
                                          {"text", title},
                                          {"textHeight", 18},
                                          {"alignSelf", OMDemiurgeAlign::FlexStart}}));
        section->mountDirect(std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get())
                                 ->style({{"color", (int)0xffffffdd},
                                          {"text", description},
                                          {"textHeight", 12},
                                          {"alignSelf", OMDemiurgeAlign::FlexStart}}));

        auto bodyWrap = std::make_shared<node::OMDemiurgePane>();
        bodyWrap->style({{"flexDirection", OMDemiurgeDirection::Column},
                         {"alignItems", OMDemiurgeAlign::Center},
                         {"flexGap", OMDemiurgeSize::pixels(6.0f)},
                         {"width", OMDemiurgeSize::fill()}});
        bodyWrap->mountDirect(body);
        section->mountDirect(bodyWrap);
        galleryList->mountDirect(section);

        auto separator = std::make_shared<node::controls::OMDemiurgeDivider>(false);
        galleryList->mountDirect(separator);
    };

    addGallerySection("ProgressBar", "Determinate and indeterminate progress states.", progressContent);
    addGallerySection("CheckBox", "Toggle a boolean option with an animated check.", checkBox);
    addGallerySection("RadioGroup", "Select one option from a group.", radioGroup);
    addGallerySection("Switch", "Animated on/off switch.", demoSwitch);
    addGallerySection("Slider", "Drag the handle to pick a value.", demoSlider);
    addGallerySection("Spinner", "Continuous loading indicator.", demoSpinner);
    addGallerySection("IconButton", "Compact button with an SVG icon.", demoIconButton);
    addGallerySection("Badge", "Small count or status badge.", demoBadge);
    addGallerySection("Link", "Text link with hover animation.", demoLink);
    demoInput->style("width", OMDemiurgeSize::fill());
    addGallerySection("TextInput", "Single-line text input.", demoInput);
    demoTextEdit->style("width", OMDemiurgeSize::fill());
    addGallerySection("TextEdit", "Multi-line text editor.", demoTextEdit);

    auto labelDemo = std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get());
    labelDemo->style({{"textHeight", 14}, {"alignSelf", OMDemiurgeAlign::FlexStart}});
    labelDemo->setText("OMDemiurgeLabel");
    labelDemo->setColor(glm::vec4(0.0f, 0.831f, 1.0f, 1.0f));

    auto multilineDemo = std::make_shared<node::controls::OMDemiurgeMultilineLabel>(fontset.get());
    multilineDemo->style({{"textHeight", 13},
                          {"textLineHeight", 17.0f},
                          {"color", (int)0xffffffff},
                          {"alignSelf", OMDemiurgeAlign::FlexStart}});
    multilineDemo->setText("MultilineLabel\nSecond line of text.\nThird line demonstrates \\n handling.");

    auto labelContent = std::make_shared<node::OMDemiurgePane>();
    labelContent->style({{"flexDirection", OMDemiurgeDirection::Column},
                         {"flexGap", OMDemiurgeSize::pixels(6.0f)},
                         {"width", OMDemiurgeSize::fill()}});
    labelContent->mountDirect(labelDemo);
    labelContent->mountDirect(multilineDemo);
    addGallerySection("Label / MultilineLabel", "Single-line and multi-line text labels.", labelContent);

    auto collapsibleDemo = std::make_shared<node::container::OMDemiurgeDropDownPane>(fontset.get());
    collapsibleDemo->style("width", OMDemiurgeSize::fill());
    collapsibleDemo->setTitle("Details");
    auto collapsibleText = std::make_shared<node::controls::OMDemiurgeMultilineLabel>(fontset.get());
    collapsibleText->style({{"textHeight", 12}, {"textLineHeight", 16.0f}, {"color", (int)0xffffffcc}});
    collapsibleText->setText("Collapsible content.\nClick the header to expand or collapse.");
    collapsibleDemo->setContentNode(collapsibleText);
    addGallerySection("DropDownPane", "Collapsible panel for secondary content.", collapsibleDemo);

    dialogDemoButton = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    dialogDemoButton->style("label", "Open Dialog");
    dialogDemoButton->style("width", OMDemiurgeSize::pixels(200.0f));
    dialogDemoButton->style("height", OMDemiurgeSize::pixels(30.0f));
    dialogDemoButton->setBackgroundColor({0.17f, 0.17f, 0.20f, 1.0f});
    dialogDemoButton->setOnClick([this]() {
        dialog->show(node, "This is a simple message dialog.\n\nIt has one button used to close it.", "Close");
    });

    auto dialogPairButton = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    dialogPairButton->style("label", "Open Two Dialogs");
    dialogPairButton->style("width", OMDemiurgeSize::pixels(200.0f));
    dialogPairButton->style("height", OMDemiurgeSize::pixels(30.0f));
    dialogPairButton->setBackgroundColor({0.17f, 0.17f, 0.20f, 1.0f});
    dialogPairButton->setOnClick([this]() {
        dialog->show(node, "First dialog.\n\nThis one is below the second dialog.", "Close");
        dialog2->show(node, "Second dialog.\n\nBoth dialogs are visible at the same time.", "Close");
    });

    auto dialogDemoRow = std::make_shared<node::OMDemiurgePane>();
    dialogDemoRow->style({{"flexDirection", OMDemiurgeDirection::Row},
                          {"alignItems", OMDemiurgeAlign::Center},
                          {"flexGap", OMDemiurgeSize::pixels(8.0f)},
                          {"width", OMDemiurgeSize::fill()}});
    dialogDemoRow->mountDirect(dialogDemoButton);
    dialogDemoRow->mountDirect(dialogPairButton);
    addGallerySection("Dialog", "Simple modal dialogs; open one or two at the same time.", dialogDemoRow);

    demoSelector = std::make_shared<node::controls::OMDemiurgeDropDownSelector>(fontset.get());
    demoSelector->style("width", OMDemiurgeSize::pixels(220.0f));
    demoSelector->setItems({"Apple", "Banana", "Cherry", "Dragonfruit", "Elderberry", "Fig", "Grape", "Honeydew",
                            "Kiwi", "Lemon", "Mango", "Nectarine"});
    demoSelector->setSelectedIndex(0, false);
    addGallerySection("Dropdown Selector", "Overlay dropdown with scrollable options.", demoSelector);

    addGallerySection("ScrollBar", "Vertical and horizontal scroll bars.", scrollContent);

    demoScrollPane = std::make_shared<node::container::OMDemiurgeScrollPane>();
    demoScrollPane->style("width", OMDemiurgeSize::fill());
    demoScrollPane->style("height", OMDemiurgeSize::fit());
    demoScrollPane->style("flexGrow", 1.0f);
    demoScrollPane->setOrientation(node::container::OMDemiurgeScrollPaneOrientation::Vertical);
    demoScrollPane->setContentStretch(true);
    demoScrollPane->setContentNode(galleryList);

    auto pageMain = std::make_shared<node::OMDemiurgePane>();
    pageMain->style(
        {{"flexDirection", Column}, {"flexGap", OMDemiurgeSize::pixels(15.0f)}, {"width", OMDemiurgeSize::fill()}});
    pageMain->mountDirect(std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get())
                              ->style({{"color", (int)0xffffffff},
                                       {"text", "OpenMinecraft"},
                                       {"textHeight", 24},
                                       {"alignSelf", OMDemiurgeAlign::FlexStart}}));
    pageMain->mountDirect(std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get())
                              ->style({{"color", (int)0xffffffff},
                                       {"text", fmt::format("Version {}-{}", OM_VERSION, OM_VERSION_CHANNEL)},
                                       {"textHeight", 12},
                                       {"border", OMDemiurgeEdgeInsets{0, 60, 0, 0}}}));
    pageMain->mountDirect(button->style("animationSpeed", 1.0f)->style("label", "Demo Test"));
    pageMain->mountDirect(button2->style("animationSpeed", 1.0f)->style("label", "Quit"));
    chunkStatusBar = std::make_shared<node::controls::OMDemiurgeProgressBar>();
    chunkStatusBar->style({
        {"width", OMDemiurgeSize::fill()},
        {"height", OMDemiurgeSize::pixels(3.0f)},
        {"radius", glm::vec4(3.0f)},
    });
    pageMain->mountDirect(chunkStatusBar);

    auto pageGallery = std::make_shared<node::OMDemiurgePane>();
    pageGallery->style({{"flexDirection", Column},
                        {"alignItems", OMDemiurgeAlign::Stretch},
                        {"flexGap", OMDemiurgeSize::pixels(12.0f)},
                        {"width", OMDemiurgeSize::fill()}});
    pageGallery->mountDirect(std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get())
                                 ->style({{"color", (int)0xffffffff},
                                          {"text", "Control Gallery"},
                                          {"textHeight", 18},
                                          {"alignSelf", OMDemiurgeAlign::FlexStart}}));
    pageGallery->mountDirect(demoScrollPane);

    auto pageMisc = std::make_shared<node::OMDemiurgePane>();
    pageMisc->style(
        {{"flexDirection", Column}, {"flexGap", OMDemiurgeSize::pixels(12.0f)}, {"width", OMDemiurgeSize::fill()}});
    pageMisc->mountDirect(std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get())
                              ->style({{"color", (int)0xffffffff},
                                       {"text", "Misc"},
                                       {"textHeight", 18},
                                       {"alignSelf", OMDemiurgeAlign::FlexStart}}));
    auto debugTreeButton = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
    debugTreeButton->style("animationSpeed", 1.0f)->style("label", "Debug UI Tree");
    debugTreeButton->style("width", OMDemiurgeSize::pixels(200.0f));
    debugTreeButton->style("textHeight", 11);
    debugTreeButton->style("alignSelf", OMDemiurgeAlign::FlexStart);
    debugTreeButton->setBackgroundColor({0.17f, 0.17f, 0.20f, 1.0f});
    debugTreeButton->setOnClick([this]() {
        if (this->node)
        {
            this->node->dumpTree();
        }
    });
    pageMisc->mountDirect(debugTreeButton);

    demoTabs = std::make_shared<node::container::OMDemiurgeTabPane>(fontset.get());
    demoTabs->style("width", OMDemiurgeSize::fill());
    demoTabs->style("height", OMDemiurgeSize::fill());
    demoTabs->setPageFill(true);
    demoTabs->addPage("Main", pageMain);
    demoTabs->addPage("Gallery", pageGallery);
    demoTabs->addPage("Misc", pageMisc);

    // Keep the window frame (radius + border) as a child of a full-screen
    // root. Overlays such as Dialog and Tooltip mount on the outer root and
    // therefore cover the whole window instead of being inset by the frame
    // border, which previously produced visible seams at the edges.
    auto windowRoot = std::make_shared<node::OMDemiurgeRectNode>()
                          ->style({
                              {"color", (int)0x2c2c3412},
                              {"flexDirection", Column},
                              {"justifyContent", SpaceBetween},
                              {"width", OMDemiurgeSize::fill()},
                              {"height", OMDemiurgeSize::fill()},
                              {"radius", glm::vec4(5.0f)},
                              {"border", OMDemiurgeEdgeInsets{10, 10, 10, 10}},
                          })
                          ->mount(std::make_shared<node::OMDemiurgePane>()
                                      ->style({
                                          {"flexDirection", Column},
                                          {"width", OMDemiurgeSize::fill()},
                                          {"height", OMDemiurgeSize::fill()},
                                          {"flexGap", 15_px},
                                          {"border", OMDemiurgeEdgeInsets{10, 10, 10, 10}},
                                      })
                                      ->mount(demoTabs));

    node = std::make_shared<node::OMDemiurgePane>()
               ->style({
                   {"flexDirection", Column},
                   {"width", OMDemiurgeSize::fill()},
                   {"height", OMDemiurgeSize::fill()},
               })
               ->mount(windowRoot);

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
    node::controls::OMDemiurgeTooltip::instance().setFontSet(nullptr);
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