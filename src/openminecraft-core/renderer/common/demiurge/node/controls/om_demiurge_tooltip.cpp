#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_tooltip.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_multiline_label.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace openminecraft::renderer::common::demiurge::node::controls
{
auto OMDemiurgeTooltip::instance() -> OMDemiurgeTooltip &
{
    static OMDemiurgeTooltip instance;
    static bool callbackBound = false;

    if (!callbackBound)
    {
        callbackBound = true;
        auto *self = &instance;
        instance.fade.onAnimateFinish = [self](float value) {
            if (self->hiding)
            {
                if (value <= 0.0001f)
                {
                    self->fadeDone = true;
                }
            }
            else
            {
                self->fading = false;
            }
        };
    }

    return instance;
}

void OMDemiurgeTooltip::setTooltip(const OMDemiurgeNode *node, const std::string &text, float delay)
{
    if (node == nullptr)
    {
        return;
    }

    if (text.empty())
    {
        detach(node);
        return;
    }

    entries[node] = Entry{text, std::max(0.0f, delay)};

    if (panel && active == node && label && shownText != text)
    {
        label->style("text", text);
        shownText = text;
    }
}

auto OMDemiurgeTooltip::getTooltip(const OMDemiurgeNode *node) const -> std::string
{
    auto it = entries.find(node);
    return it == entries.end() ? std::string() : it->second.text;
}

void OMDemiurgeTooltip::setTooltipDelay(const OMDemiurgeNode *node, float seconds)
{
    auto it = entries.find(node);

    if (it != entries.end())
    {
        it->second.delay = std::max(0.0f, seconds);
    }
}

auto OMDemiurgeTooltip::getTooltipDelay(const OMDemiurgeNode *node) const -> float
{
    auto it = entries.find(node);
    return it == entries.end() ? 0.6f : it->second.delay;
}

void OMDemiurgeTooltip::detach(const OMDemiurgeNode *node)
{
    entries.erase(node);

    if (active == node)
    {
        active = nullptr;
        hovered = false;
        startFade(0.0f);
    }
}

void OMDemiurgeTooltip::hideFor(const OMDemiurgeNode *node)
{
    if (active == node)
    {
        hovered = false;
        active = nullptr;
        startFade(0.0f);
    }
}

void OMDemiurgeTooltip::setFontSet(geom::OMFontSet *value)
{
    fontset = value;
}

void OMDemiurgeTooltip::setPointer(float x, float y)
{
    pointerX = x;
    pointerY = y;
    pointerValid = true;
}

void OMDemiurgeTooltip::startFade(float target)
{
    fadeDone = false;
    hiding = target <= 0.0f;

    if (hiding)
    {
        fade.animateTo(0.0f, animation::easeInCubic<float>, 0.12f);
    }
    else
    {
        fade.animateTo(1.0f, animation::easeOutCubic<float>, 0.12f);
    }

    fading = true;
}

void OMDemiurgeTooltip::applyOpacity(float opacity)
{
    opacity = std::clamp(opacity, 0.0f, 1.0f);

    if (std::abs(opacity - appliedOpacity) < 0.001f)
    {
        return;
    }

    appliedOpacity = opacity;

    if (panel)
    {
        panel->style("opacity", opacity);
    }

    if (label)
    {
        label->style("opacity", opacity);
    }
}

auto OMDemiurgeTooltip::findTarget(const std::shared_ptr<OMDemiurgeNode> &root) -> OMDemiurgeNode *
{
    if (!pointerValid)
    {
        return active;
    }

    OMDemiurgeNode *best = nullptr;
    float bestArea = std::numeric_limits<float>::max();

    for (auto &entry : entries)
    {
        auto *candidate = const_cast<OMDemiurgeNode *>(entry.first);

        if (candidate->getRoot() != root || !candidate->isHoverableAt(pointerX, pointerY))
        {
            continue;
        }

        try
        {
            const auto bound = candidate->fetchLayoutBound();
            const float area = std::max(0.0f, bound.width) * std::max(0.0f, bound.height);

            if (best == nullptr || area < bestArea)
            {
                best = candidate;
                bestArea = area;
            }
        }
        catch (const std::bad_any_cast &)
        {
        }
    }

    return best;
}

auto OMDemiurgeTooltip::isHovered(const OMDemiurgeNode *node) const -> bool
{
    return active == node && hovered;
}

void OMDemiurgeTooltip::onHover(const OMDemiurgeNode *node, bool entered, float x, float y)
{
    // Target selection is fully pointer-driven in update(). Enter/exit events
    // are still useful for keeping the pointer position fresh, but they must not
    // overwrite the target: a synthetic refreshHover() enter on an ancestor used
    // to replace the inner control every frame and reset the hover delay.
    setPointer(x, y);
}

void OMDemiurgeTooltip::dismiss()
{
    // Hide the visible panel; the active target is kept and the pointer scan in
    // update() will re-arm it after the interaction ends.
    if (panel && !hiding)
    {
        startFade(0.0f);
    }
}

void OMDemiurgeTooltip::resume()
{
    // Restart the delay for the target under the pointer. The next update()
    // derives the actual target from the pointer position.
    if (active && hovered)
    {
        hoverStart = std::chrono::steady_clock::now();
    }
}

void OMDemiurgeTooltip::hide()
{
    auto oldRoot = panelRoot.lock();
    auto oldPanel = panel;
    panel.reset();
    label.reset();
    shownText.clear();
    panelRoot.reset();
    fade.setTo(0.0f);
    appliedOpacity = -1.0f;
    fading = false;
    hiding = false;
    fadeDone = false;

    if (oldPanel && oldRoot)
    {
        oldRoot->umount(oldPanel);
    }
}

void OMDemiurgeTooltip::update(const std::shared_ptr<OMDemiurgeNode> &root)
{
    if (!root)
    {
        return;
    }

    // A tooltip belongs to exactly one UI tree. Debug HUD and Surface UI both
    // tick the shared manager; only the owner of the active target/panel may
    // mutate it.
    if (active)
    {
        if (active->getRoot() != root)
        {
            return;
        }
    }
    else if (panel)
    {
        auto currentRoot = panelRoot.lock();

        if (currentRoot && currentRoot != root)
        {
            return;
        }
    }

    // Modal overlays (DropDownSelector list, dialogs) own the interaction layer;
    // never draw a tooltip above them.
    if (root->hasModalChild())
    {
        active = nullptr;
        hovered = false;
        hide();
        return;
    }

    // Derive the target from the pointer every frame. This does not depend on
    // enter/exit events, which can be missed after clicks, captures or layout
    // movement.
    if (pointerValid)
    {
        auto *candidate = findTarget(root);

        if (candidate != active)
        {
            if (panel && !hiding)
            {
                startFade(0.0f);
            }

            active = candidate;
            hoverStart = std::chrono::steady_clock::now();
        }

        hovered = candidate != nullptr;
    }

    const bool ownsPanel = panel && panelRoot.lock() == root;

    if (ownsPanel && fading)
    {
        applyOpacity(fade.get());

        if (hiding && fadeDone)
        {
            hide();
            return;
        }
    }

    if (OMDemiurgeNode::fetchMouseCapture() != nullptr)
    {
        if (ownsPanel && !hiding)
        {
            startFade(0.0f);
        }

        return;
    }

    if (!active)
    {
        if (ownsPanel && !hiding)
        {
            startFade(0.0f);
        }

        return;
    }

    if (active->getRoot() != root)
    {
        return;
    }

    auto it = entries.find(active);

    if (it == entries.end() || it->second.text.empty() || fontset == nullptr || !hovered)
    {
        if (ownsPanel && !hiding)
        {
            startFade(0.0f);
        }

        return;
    }

    const auto elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - hoverStart).count();

    if (elapsed < it->second.delay)
    {
        return;
    }

    if (!panel)
    {
        auto tooltipPanel = std::make_shared<node::OMDemiurgeRectNode>();
        tooltipPanel->style({
            {"position", OMDemiurgePosition::Absolute},
            {"width", OMDemiurgeSize::fit()},
            {"height", OMDemiurgeSize::fit()},
            {"color", static_cast<int>(f4ToRgba(glm::vec4(0.055f, 0.055f, 0.075f, 0.96f)))},
            {"radius", glm::vec4(4.0f)},
            {"padding", OMDemiurgeEdgeInsets{5, 9, 5, 9}},
        });

        auto tooltipLabel = std::make_shared<OMDemiurgeMultilineLabel>(fontset);
        tooltipLabel->style({{"text", it->second.text},
                             {"textHeight", 12},
                             {"textLineHeight", 15.0f},
                             {"color", static_cast<int>(f4ToRgba(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)))},
                             {"factor", 0.02f}});
        tooltipPanel->mountDirect(tooltipLabel);
        tooltipPanel->setLayerOverlay(true);

        panel = tooltipPanel;
        label = tooltipLabel;
        panelRoot = root;
        shownText = it->second.text;
        appliedOpacity = -1.0f;
        fade.setTo(0.0f);
        applyOpacity(0.0f);
        root->mountDirect(panel);
        startFade(1.0f);
    }
    else
    {
        if (hiding)
        {
            startFade(1.0f);
        }

        if (label && shownText != it->second.text)
        {
            label->style("text", it->second.text);
            shownText = it->second.text;
        }
    }

    if (!panel)
    {
        return;
    }

    const auto rootBound = root->fetchLayoutBound();
    const auto targetBound = active->fetchLayoutBound();
    const float anchorX = pointerValid ? pointerX : targetBound.x + targetBound.width * 0.5f;
    const float anchorY = pointerValid ? pointerY : targetBound.y + targetBound.height * 0.5f;
    int lines = 1;

    for (char c : it->second.text)
    {
        if (c == '\n')
        {
            ++lines;
        }
    }

    const float estimatedHeight = static_cast<float>(lines) * 15.0f + 10.0f;

    float localX = std::max(0.0f, anchorX - rootBound.x + 12.0f);
    float localY = anchorY - rootBound.y - estimatedHeight - 8.0f;

    if (localY < 0.0f)
    {
        localY = anchorY - rootBound.y + 18.0f;
    }

    try
    {
        const auto panelBound = panel->fetchLayoutBound();

        if (panelBound.width > 0.0f && rootBound.width > 0.0f)
        {
            const float minX = 4.0f;
            const float maxX = std::max(minX, rootBound.width - panelBound.width - 4.0f);
            localX = std::clamp(anchorX - rootBound.x + 12.0f, minX, maxX);
        }
    }
    catch (const std::bad_any_cast &)
    {
    }

    panel->style("offsetX", OMDemiurgeSize::pixels(localX))->style("offsetY", OMDemiurgeSize::pixels(localY));
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
