#ifndef OM_DEMIURGE_PROGRESSBAR_HPP
#define OM_DEMIURGE_PROGRESSBAR_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_control.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/container/om_demiurge_pane.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <limits>
#include <memory>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Indeterminate progress bar: a rounded track with a constant-length pill that
// sweeps back and forth and gets squashed against the track ends.
class OMDemiurgeProgressBar : public OMDemiurgePane, public OMDemiurgeControl
{
  public:
    OMDemiurgeProgressBar();
    ~OMDemiurgeProgressBar() override;

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override;
    void update() override;

    void setTrackColor(glm::vec4 color);
    void setBarColor(glm::vec4 color);
    // Error state tints the bar with errorColor (soft red by default).
    void setError(bool value, bool animate = true);
    auto isError() const -> bool
    {
        return error;
    }
    void setErrorColor(glm::vec4 color);
    void setAnimationSpeed(float seconds);

    // 0..1 => determinate; anything outside (or NaN) => indeterminate.
    void setProgress(float progress);
    void setIndeterminate(bool value)
    {
        setProgress(value ? std::numeric_limits<float>::quiet_NaN() : 0.0f);
    }
    auto isIndeterminate() const -> bool
    {
        return !determinate;
    }
    void setDisabled(bool value, bool animate = true) override;
    auto isDisabled() const -> bool
    {
        return disabled;
    }
    auto isDeterminate() const -> bool
    {
        return determinate;
    }
    auto getProgress() const -> float
    {
        return determinate ? targetProgress : std::numeric_limits<float>::quiet_NaN();
    }

    animation::OMAnimationValue<glm::vec4> trackColor;
    animation::OMAnimationValue<glm::vec4> barColor;
    animation::OMAnimationValue<glm::vec4> errorColor;
    animation::OMAnimationValue<float> errorAmount;
    animation::OMAnimationValue<float> disabledAmount;

  private:
    std::shared_ptr<OMDemiurgeNode> trackNode;
    std::shared_ptr<OMDemiurgeNode> barNode;
    std::chrono::steady_clock::time_point startedAt;
    std::chrono::steady_clock::time_point lastUpdate;

    bool determinate = false;
    bool disabled = false;
    bool error = false;
    float targetProgress = 0.0f;
    float visualLeft = 0.0f;
    float visualWidth = 0.0f;
    bool blendActive = true;
    std::chrono::steady_clock::time_point blendStart;
    float blendFromLeft = 0.0f;
    float blendFromWidth = 0.0f;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
