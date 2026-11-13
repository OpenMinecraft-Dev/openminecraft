#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_dialog.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_button.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_multiline_label.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include <algorithm>

namespace openminecraft::renderer::common::demiurge::node::controls
{
namespace
{
// Ticks the open/close animation from syncLayout(), so the panel offset and
// opacity are applied before Yoga computes layout for the frame.
class OMDemiurgeDialogLayerNode : public node::container::OMDemiurgePane
{
  public:
    std::function<void()> tick;

    auto syncLayout() -> void override
    {
        if (tick)
        {
            tick();
        }

        OMDemiurgePane::syncLayout();
    }

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override
    {
        // Overlay z-order comes directly from the modal stack. There is no
        // full-screen backdrop; only the dialog panel is rendered.
        int index = -1;

        if (auto root = getRoot())
        {
            index = root->modalIndex(this);
        }

        const uint32_t zorder = static_cast<uint32_t>(std::max(0, index) + 1);

        for (auto &child : children)
        {
            child->submit(handler, withLayerZOrder(overlayLayer(0), zorder));
        }
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
        auto children = parent->fetchChildren();
        const bool mounted = std::find(children.begin(), children.end(), layerNode) != children.end();

        if (!mounted)
        {
            if (auto oldRoot = layerNode->getRoot())
            {
                oldRoot->clearModalChild(layerNode);
                oldRoot->umount(layerNode);
            }

            layerNode.reset();
            panelNode.reset();
            messageNode.reset();
            closeButton.reset();
            closing = false;
            cleanupScheduled = false;
        }
        else
        {
            // A selector popup may have temporarily owned the modal slot.
            // Showing the dialog again must always re-register it as the
            // active modal instead of silently returning while it is stale.
            openRequested = true;
            parent->setModalChild(layerNode);
            panelNode->setOpacity(1.0f);

            if (closing)
            {
                closing = false;
                cleanupScheduled = false;
                openAmount.animateTo(1.0f, animation::easeOutCubic<float>, 0.18f);
            }
            else if (openAmount.get() <= 0.001f)
            {
                openAmount.animateTo(1.0f, animation::easeOutCubic<float>, 0.18f);
            }

            messageNode->style("text", message);

            if (closeButton)
            {
                closeButton->style("label", closeLabel);
            }

            return;
        }
    }

    openRequested = true;
    const uint64_t request = ++generation;

    auto guard = alive;
    parent->deferAction([this, parent, request, guard]() {
        if (!*guard || !openRequested || request != generation)
        {
            return;
        }

        auto layer = std::make_shared<OMDemiurgeDialogLayerNode>();
        layer->tick = [this]() { updateAnimation(); };
        layer->style({
            {"position", OMDemiurgePosition::Absolute},
            {"width", OMDemiurgeSize::fill()},
            {"height", OMDemiurgeSize::fill()},
            {"flexDirection", OMDemiurgeDirection::Column},
            {"justifyContent", OMDemiurgeAlign::Center},
            {"alignItems", OMDemiurgeAlign::Center},
        });
        layer->setLayerOverlay(true);

        auto panel = std::make_shared<node::OMDemiurgeRectNode>();
        panel->style({
            {"width", OMDemiurgeSize::pixels(340.0f)},
            {"height", OMDemiurgeSize::fit()},
            {"color", static_cast<int>(f4ToRgba(glm::vec4(0.075f, 0.075f, 0.095f, 1.0f)))},
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
        panel->setOpacity(1.0f);
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

    panelNode->style("offsetX", OMDemiurgeSize::pixels(0.0f));
    panelNode->style("offsetY", OMDemiurgeSize::pixels((1.0f - amount) * 18.0f));
    // Only the close fade uses panel opacity. On open the panel is immediately
    // opaque, so a stalled/zero openAmount cannot leave an interactive but
    // invisible dialog in the tree.
    panelNode->setOpacity(closing ? amount : 1.0f);

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
