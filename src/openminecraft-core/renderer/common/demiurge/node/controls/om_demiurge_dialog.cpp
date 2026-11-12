#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_dialog.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_multiline_label.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include <algorithm>

namespace openminecraft::renderer::common::demiurge::node::controls
{
namespace
{
// Ticks the open/close animation from syncLayout(), so the panel offset and
// backdrop alpha are applied before Yoga computes layout for the frame.
class DialogLayerNode : public node::OMDemiurgeRectNode
{
  public:
    std::function<void()> tick;

    auto syncLayout() -> void override
    {
        if (tick)
        {
            tick();
        }

        OMDemiurgeRectNode::syncLayout();
    }

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override
    {
        // Put every dialog in its own overlay layer range according to its
        // position in the modal stack. This keeps a newer dialog's panel/text
        // above an older dialog's text even though text channels are submitted
        // after rect channels.
        int index = -1;

        if (auto root = getRoot())
        {
            index = root->modalIndex(this);
        }

        const uint32_t base = static_cast<uint32_t>(std::max(0, index) + 1) * 8;
        OMDemiurgeRectNode::submit(handler, overlayLayer(base));
    }
};
} // namespace

OMDemiurgeDialog::OMDemiurgeDialog(geom::OMFontSet *fontset) : fontset(fontset)
{
}

OMDemiurgeDialog::~OMDemiurgeDialog()
{
    *alive = false;

    if (auto oldRoot = root.lock(); layerNode && oldRoot)
    {
        oldRoot->clearModalChild(layerNode);

        oldRoot->umount(layerNode);
    }
}

void OMDemiurgeDialog::show(const std::shared_ptr<OMDemiurgeNode> &parent, const std::string &message,
                            const std::string &closeLabel)
{
    if (!parent || fontset == nullptr || message.empty())
    {
        return;
    }

    currentMessage = message;
    currentCloseLabel = closeLabel;
    root = parent;

    if (layerNode)
    {
        if (closing)
        {
            closing = false;
            cleanupScheduled = false;
            openRequested = true;
            openAmount.animateTo(1.0f, animation::easeOutCubic<float>, 0.18f);
        }

        messageNode->style("text", message);

        if (closeButton)
        {
            closeButton->style("label", closeLabel);
        }

        return;
    }

    openRequested = true;
    const uint64_t request = ++generation;

    auto guard = alive;
    parent->deferAction([this, parent, request, guard]() {
        if (!*guard || !openRequested || request != generation)
        {
            return;
        }

        auto layer = std::make_shared<DialogLayerNode>();
        layer->tick = [this]() { updateAnimation(); };
        layer->style({
            {"position", OMDemiurgePosition::Absolute},
            {"width", OMDemiurgeSize::fill()},
            {"height", OMDemiurgeSize::fill()},
            {"color", static_cast<int>(f4ToRgba(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f)))},
            {"flexDirection", OMDemiurgeDirection::Column},
            {"justifyContent", OMDemiurgeAlign::Center},
            {"alignItems", OMDemiurgeAlign::Center},
            {"padding", OMDemiurgeEdgeInsets{24, 24, 24, 24}},
        });
        layer->setLayerOverlay(true);

        auto panel = std::make_shared<node::OMDemiurgeRectNode>();
        panel->style({
            {"width", OMDemiurgeSize::pixels(340.0f)},
            {"height", OMDemiurgeSize::fit()},
            {"color", static_cast<int>(f4ToRgba(glm::vec4(0.075f, 0.075f, 0.095f, 0.98f)))},
            {"radius", glm::vec4(8.0f)},
            {"padding", OMDemiurgeEdgeInsets{16, 16, 16, 16}},
            {"flexDirection", OMDemiurgeDirection::Column},
            {"flexGap", OMDemiurgeSize::pixels(12.0f)},
            {"offsetY", OMDemiurgeSize::pixels(18.0f)},
        });

        auto messageLabel = std::make_shared<OMDemiurgeMultilineLabel>(fontset);
        messageLabel->style({
            {"text", currentMessage},
            {"textHeight", 14},
            {"textLineHeight", 18.0f},
            {"color", static_cast<int>(f4ToRgba(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)))},
            {"factor", 0.025f},
            {"width", OMDemiurgeSize::fill()},
        });

        auto button = std::make_shared<OMDemiurgeButton>(fontset);
        button->style({
            {"label", currentCloseLabel},
            {"width", OMDemiurgeSize::pixels(84.0f)},
            {"height", OMDemiurgeSize::pixels(28.0f)},
            {"alignSelf", OMDemiurgeAlign::FlexEnd},
        });
        button->findChildById("text")->style("textHeight", 12);
        button->findChildById("background")->style("radius", glm::vec4(4.0f));
        button->setBackgroundColor(glm::vec4(0.16f, 0.34f, 0.46f, 1.0f));
        button->setOnClick([this]() { close(); });

        panel->mountDirect(messageLabel);
        panel->mountDirect(button);
        layer->mountDirect(panel);

        parent->mountDirect(layer);
        parent->setModalChild(layer);

        layerNode = layer;
        panelNode = panel;
        messageNode = messageLabel;
        closeButton = button;
        openAmount.setTo(0.0f);
        openAmount.animateTo(1.0f, animation::easeOutCubic<float>, 0.18f);
        button->requestFocus();
    });
}

void OMDemiurgeDialog::close()
{
    if (closing)
    {
        return;
    }

    if (!layerNode)
    {
        openRequested = false;
        ++generation;
        return;
    }

    openRequested = false;
    closing = true;
    cleanupScheduled = false;
    openAmount.animateTo(0.0f, animation::easeInCubic<float>, 0.18f);

    if (onClose)
    {
        onClose();
    }
}

void OMDemiurgeDialog::updateAnimation()
{
    if (!layerNode || !panelNode)
    {
        return;
    }

    const float amount = std::clamp(openAmount.get(), 0.0f, 1.0f);
    layerNode->style("color", static_cast<int>(f4ToRgba(glm::vec4(0.0f, 0.0f, 0.0f, 0.62f * amount))));

    int stackIndex = 0;

    if (auto root = layerNode->getRoot())
    {
        stackIndex = std::max(0, root->modalIndex(layerNode.get()));
    }

    panelNode->style("offsetY", OMDemiurgeSize::pixels((1.0f - amount) * 18.0f));
    panelNode->setOpacity(amount);

    if (!closing || amount > 0.001f || cleanupScheduled)
    {
        return;
    }

    auto parent = root.lock();
    auto layer = layerNode;

    if (!parent || !layer)
    {
        return;
    }

    cleanupScheduled = true;
    auto guard = alive;
    parent->deferAction([this, guard, parent, layer]() {
        if (!*guard || !closing)
        {
            return;
        }

        parent->clearModalChild(layer);
        parent->umount(layer);

        if (layerNode == layer)
        {
            layerNode.reset();
            panelNode.reset();
            messageNode.reset();
            closeButton.reset();
        }

        openRequested = false;
        closing = false;
        cleanupScheduled = false;
    });
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
