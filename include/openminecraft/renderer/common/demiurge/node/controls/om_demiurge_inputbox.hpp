#ifndef OM_DEMIURGE_INPUTBOX_HPP
#define OM_DEMIURGE_INPUTBOX_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <chrono>
#include <functional>
#include <memory>
#include <string>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Single-line text input with caret, placeholder and horizontal scrolling.
class OMDemiurgeInputBox : public OMDemiurgeRectNode
{
  public:
    explicit OMDemiurgeInputBox(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeInputBox() override;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    void update() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processKeyDown(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult override;
    auto processTextInput(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult override;
    auto processFocusGained() -> void override;
    auto processFocusLost() -> void override;

    void setText(const std::string &text);
    auto getText() const -> const std::string &
    {
        return text;
    }
    void setPlaceholder(const std::string &placeholder);
    void setMaxLength(int codepoints);
    void setOnChange(std::function<void(const std::string &)> handler);
    void setOnSubmit(std::function<void(const std::string &)> handler);
    void setTextHeight(int height);
    void setColors(glm::vec4 background, glm::vec4 text, glm::vec4 caret);
    void setDisabled(bool value, bool animate = true);
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    // Global flag used by the application to suppress gameplay keys (WASD/Space)
    // while a text input is focused.
    static auto isTextInputFocused() -> bool
    {
        return activeInput.lock() != nullptr;
    }

    animation::OMAnimationValue<float> focusAmount;
    animation::OMAnimationValue<float> disabledAmount;
    animation::OMAnimationValue<glm::vec4> backgroundColor;
    animation::OMAnimationValue<glm::vec4> textColor;
    animation::OMAnimationValue<glm::vec4> caretColor;
    animation::OMAnimationValue<glm::vec4> placeholderColor;

  private:
    auto measure(const std::string &value) -> float;
    auto caretFromX(float x) -> size_t;
    auto scrollForCaret() -> float;
    auto insertText(const std::string &value) -> void;
    auto notifyChange() -> void;

    std::shared_ptr<OMDemiurgeNode> textNode;
    std::shared_ptr<OMDemiurgeNode> caretNode;

    geom::OMFontSet *fontset = nullptr;
    std::string text;
    std::string placeholder = "";
    size_t caret = 0;
    int maxLength = 256;
    bool disabled = false;
    std::chrono::steady_clock::time_point blinkStart = std::chrono::steady_clock::now();
    float lastScroll = 0.0f;

    std::function<void(const std::string &)> handleChange = [](const std::string &) {};
    std::function<void(const std::string &)> handleSubmit = [](const std::string &) {};

    inline static std::weak_ptr<OMDemiurgeInputBox> activeInput;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
