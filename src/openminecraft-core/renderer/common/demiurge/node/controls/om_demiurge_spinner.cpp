#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_spinner.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>
#include <cmath>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
static constexpr float SpinnerSize = 20.0f;
static constexpr float BarWidth = 2.0f;
static constexpr float BarHeight = 5.0f;

OMDemiurgeSpinner::OMDemiurgeSpinner() : dotColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)), disabledAmount(0.0f)
{
    stylesStorage.put("width", OMDemiurgeSize::pixels(SpinnerSize));
    stylesStorage.put("height", OMDemiurgeSize::pixels(SpinnerSize));
    rebuildDots();
}

OMDemiurgeSpinner::~OMDemiurgeSpinner() = default;

void OMDemiurgeSpinner::rebuildDots()
{
    for (auto &dot : dots)
    {
        this->umount(dot);
    }

    dots.clear();

    for (int i = 0; i < dotCount; ++i)
    {
        auto dot = std::make_shared<OMDemiurgeRectNode>()->style({
            {"position", OMDemiurgePosition::Absolute},
            {"offsetX", OMDemiurgeSize::pixels(0.0f)},
            {"offsetY", OMDemiurgeSize::pixels(0.0f)},
            {"width", OMDemiurgeSize::pixels(BarWidth)},
            {"height", OMDemiurgeSize::pixels(BarHeight)},
            {"radius", glm::vec4(BarWidth * 0.5f)},
        });

        this->mountDirect(dot);
        dots.push_back(dot);
    }
}

auto OMDemiurgeSpinner::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    OMDemiurgeContainerNode::submit(handler, depth);
}

void OMDemiurgeSpinner::update()
{
    static const auto startedAt = std::chrono::steady_clock::now();
    const float t = std::chrono::duration<float>(std::chrono::steady_clock::now() - startedAt).count();

    float width = SpinnerSize;
    float height = SpinnerSize;

    try
    {
        auto bound = fetchLayoutBound();

        if (bound.width > 0.0f && bound.height > 0.0f)
        {
            width = bound.width;
            height = bound.height;
        }
    }
    catch (std::bad_any_cast &)
    {
    }

    const float radius = std::max(0.0f, std::min(width, height) * 0.5f - BarHeight * 0.5f);
    const float centerX = width * 0.5f;
    const float centerY = height * 0.5f;

    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);
    const auto base = dotColor.get();

    // macOS-style: discrete steps (ticking) instead of continuous rotation.
    const int count = std::max(1, static_cast<int>(dots.size()));
    const int step = static_cast<int>(t * 8.0f);
    const float stepAngle = 6.2831853f / static_cast<float>(count);
    const float baseAngle = static_cast<float>(step) * stepAngle;

    for (int i = 0; i < count; ++i)
    {
        const float phase = static_cast<float>(i) / static_cast<float>(count);
        const float angle = baseAngle - phase * 6.2831853f;
        const float alpha = 0.12f + 0.88f * std::pow(1.0f - phase, 2.0f);

        const float x = centerX + std::cos(angle) * radius - BarWidth * 0.5f;
        const float y = centerY + std::sin(angle) * radius - BarHeight * 0.5f;

        glm::vec3 rgb = glm::mix(glm::vec3(base), glm::vec3(0.55f), disabledValue * 0.75f);

        dots[i]->style("offsetX", OMDemiurgeSize::pixels(x))
            ->style("offsetY", OMDemiurgeSize::pixels(y))
            ->style("rotation", angle - 1.5707963f)
            ->style("color", (int)f4ToRgba(glm::vec4(rgb, base.a * alpha * (1.0f - disabledValue * 0.5f))));
    }

    OMDemiurgeContainerNode::update();
}

void OMDemiurgeSpinner::setColor(glm::vec4 color)
{
    dotColor.animateTo(color, easeOutCirc<float>, 0.18f);
}

void OMDemiurgeSpinner::setSpeed(float radiansPerSecond)
{
    spinSpeed = radiansPerSecond;
}

void OMDemiurgeSpinner::setDotCount(int count)
{
    dotCount = std::clamp(count, 3, 16);
    rebuildDots();
}

void OMDemiurgeSpinner::setDisabled(bool value, bool animate)
{
    if (disabled == value)
    {
        return;
    }

    disabled = value;

    if (animate)
    {
        disabledAmount.animateTo(disabled ? 1.0f : 0.0f, easeOutCirc<float>, 0.18f);
    }
    else
    {
        disabledAmount.setTo(disabled ? 1.0f : 0.0f);
    }
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
