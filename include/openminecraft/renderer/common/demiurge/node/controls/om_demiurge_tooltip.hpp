#ifndef OM_DEMIURGE_TOOLTIP_HPP
#define OM_DEMIURGE_TOOLTIP_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>

namespace openminecraft::geom
{
class OMFontSet;
}

namespace openminecraft::renderer::common::demiurge
{
class OMDemiurgeNode;
}

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Shared tooltip manager. Tooltip data and the visible panel live here instead
// of on OMDemiurgeNode, and the panel is mounted directly under the UI root so
// ancestor clipping (ScrollPane etc.) cannot cut it off.
class OMDemiurgeTooltip
{
  public:
    static auto instance() -> OMDemiurgeTooltip &;

    void setTooltip(const OMDemiurgeNode *node, const std::string &text, float delay = 0.6f);
    void setImmediateTooltip(const OMDemiurgeNode *node, const std::string &text);
    auto isImmediate(const OMDemiurgeNode *node) const -> bool;
    auto getTooltip(const OMDemiurgeNode *node) const -> std::string;
    void setTooltipDelay(const OMDemiurgeNode *node, float seconds);
    auto getTooltipDelay(const OMDemiurgeNode *node) const -> float;
    void detach(const OMDemiurgeNode *node);
    void hideFor(const OMDemiurgeNode *node);

    void setFontSet(geom::OMFontSet *fontset);
    void setPointer(float x, float y);
    void onHover(const OMDemiurgeNode *node, bool entered, float x, float y);
    auto isHovered(const OMDemiurgeNode *node) const -> bool;

    // Called once per frame by OMDemiurgeRendererHandler. `root` is the root of
    // the tree being rendered; only the handler owning the active target mounts
    // the panel.
    void update(const std::shared_ptr<OMDemiurgeNode> &root);
    void hide();
    void dismiss();
    void resume();

  private:
    struct Entry
    {
        std::string text;
        float delay = 0.6f;
        bool immediate = false;
    };

    void startFade(float target);
    auto findTarget(const std::shared_ptr<OMDemiurgeNode> &root) -> OMDemiurgeNode *;
    void applyOpacity(float opacity);

    std::unordered_map<const OMDemiurgeNode *, Entry> entries;
    OMDemiurgeNode *active = nullptr;
    bool hovered = false;

    std::chrono::steady_clock::time_point hoverStart;

    float pointerX = 0.0f;
    float pointerY = 0.0f;
    bool pointerValid = false;

    std::shared_ptr<OMDemiurgeNode> panel;
    std::shared_ptr<OMDemiurgeNode> label;
    std::weak_ptr<OMDemiurgeNode> panelRoot;
    std::string shownText;

    animation::OMAnimationValue<float> fade{0.0f};
    float appliedOpacity = -1.0f;
    bool fading = false;
    bool hiding = false;
    bool fadeDone = false;

    geom::OMFontSet *fontset = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
