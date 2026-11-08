#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_progressbar.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

#include <algorithm>
#include <array>
#include <cmath>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
OMDemiurgeProgressBar::OMDemiurgeProgressBar()
    : trackColor(glm::vec4(0.106f, 0.106f, 0.141f, 0.25f)), barColor(glm::vec4(0.0f, 0.831f, 1.0f, 1.0f))
{
    startedAt = std::chrono::steady_clock::now();
    lastUpdate = startedAt;
    blendStart = startedAt;

    stylesStorage.put("width", OMDemiurgeSize::pixels(180.0f));
    stylesStorage.put("height", OMDemiurgeSize::pixels(8.0f));
    stylesStorage.put("radius", glm::vec4(4.0f));

    trackNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
        {"offsetY", OMDemiurgeSize::pixels(0.0f)},
        {"width", OMDemiurgeSize::fill()},
        {"height", OMDemiurgeSize::fill()},
    });

    barNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
        {"offsetY", OMDemiurgeSize::pixels(0.0f)},
        {"width", OMDemiurgeSize::pixels(64.0f)},
        {"height", OMDemiurgeSize::fill()},
    });

    this->mountDirect(trackNode);
    this->mountDirect(barNode);
}

OMDemiurgeProgressBar::~OMDemiurgeProgressBar() = default;

auto OMDemiurgeProgressBar::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    OMDemiurgeContainerNode::submit(handler, depth);
}

void OMDemiurgeProgressBar::update()
{
    const auto radius = stylesStorage.get<glm::vec4>("radius", glm::vec4(4.0f));

    trackNode->style("color", (int)f4ToRgba(trackColor.get()))->style("radius", radius);
    barNode->style("color", (int)f4ToRgba(barColor.get()))->style("radius", radius);

    // Track width in pixels (falls back to the style value before the first layout pass).
    float trackWidth = 180.0f;

    try
    {
        auto bound = fetchLayoutBound();

        if (bound.width > 0.0f)
        {
            trackWidth = bound.width;
        }
    }
    catch (std::bad_any_cast &)
    {
    }

    const float segmentLength = std::max(4.0f, stylesStorage.get<float>("segment_length", 64.0f));
    const float squeeze = std::max(0.0f, stylesStorage.get<float>("squeeze", 30.0f));
    const float minLength = std::max(1.0f, stylesStorage.get<float>("min_length", 12.0f));
    const float cycle = std::max(0.05f, stylesStorage.get<float>("cycle", 1.15f));

    const auto now = std::chrono::steady_clock::now();
    const float dt = std::clamp(std::chrono::duration<float>(now - lastUpdate).count(), 0.0f, 0.1f);
    lastUpdate = now;

    const float t = std::chrono::duration<float>(now - startedAt).count();

    float targetLeft = 0.0f;
    float targetWidth = std::max(segmentLength, minLength);

    if (determinate)
    {
        targetLeft = 0.0f;
        targetWidth = std::max(0.0f, trackWidth * std::clamp(targetProgress, 0.0f, 1.0f));
    }
    else
    {
        // Ping-pong: one leg left->right, the next right->left.
        const float phase = std::fmod(t / cycle, 2.0f);
        const float triangle = phase <= 1.0f ? phase : 2.0f - phase;
        const float u = easeInOutSine(std::clamp(triangle, 0.0f, 1.0f));

        // Squash: allow the pill to push past each end, then fold the overshoot back in.
        const float base = trackWidth - segmentLength;
        const float rawX = -squeeze + (base + squeeze * 2.0f) * u;

        targetLeft = rawX;
        targetWidth = segmentLength;

        if (targetLeft < 0.0f)
        {
            targetLeft = 0.0f;
            targetWidth = segmentLength + rawX;
        }

        if (targetLeft + targetWidth > trackWidth)
        {
            targetWidth = trackWidth - targetLeft;
        }

        targetWidth = std::max(targetWidth, minLength);
    }

    // Smoothing: exponential follow for determinate updates, and an easeOutCubic
    // blend when switching modes, so the bar never jumps.
    if (blendActive)
    {
        const float blend = std::clamp(
            std::chrono::duration<float>(now - blendStart).count() / 0.28f, 0.0f, 1.0f);
        const float e = easeOutCubic(blend);

        visualLeft = blendFromLeft * (1.0f - e) + targetLeft * e;
        visualWidth = blendFromWidth * (1.0f - e) + targetWidth * e;

        if (blend >= 1.0f)
        {
            blendActive = false;
        }
    }
    else if (determinate)
    {
        const float k = 1.0f - std::exp(-dt / 0.09f);
        visualLeft += (targetLeft - visualLeft) * k;
        visualWidth += (targetWidth - visualWidth) * k;
    }
    else
    {
        visualLeft = targetLeft;
        visualWidth = targetWidth;
    }

    barNode->style("offsetX", OMDemiurgeSize::pixels(visualLeft))
        ->style("width", OMDemiurgeSize::pixels(std::max(0.0f, visualWidth)));

    OMDemiurgeContainerNode::update();
}

void OMDemiurgeProgressBar::setTrackColor(glm::vec4 color)
{
    trackColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.2f));
}

void OMDemiurgeProgressBar::setBarColor(glm::vec4 color)
{
    barColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animation_speed", 0.2f));
}

void OMDemiurgeProgressBar::setProgress(float progress)
{
    const bool nextDeterminate = std::isfinite(progress) && progress >= 0.0f && progress <= 1.0f;

    if (nextDeterminate != determinate)
    {
        // Start a short blend from the current visual state to the new mode.
        blendActive = true;
        blendStart = std::chrono::steady_clock::now();
        blendFromLeft = visualLeft;
        blendFromWidth = visualWidth;
    }

    determinate = nextDeterminate;

    if (nextDeterminate)
    {
        targetProgress = progress;
    }
}

void OMDemiurgeProgressBar::setAnimationSpeed(float seconds)
{
    stylesStorage.put("animation_speed", seconds);
}
} // namespace openminecraft::renderer::common::demiurge::node::controls
