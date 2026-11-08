#ifndef OM_DEMIURGE_TEXTEDIT_HPP
#define OM_DEMIURGE_TEXTEDIT_HPP

#include "glm/ext/vector_float4.hpp"
#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/animation/om_animation_value.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_rect.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace openminecraft::renderer::common::demiurge::node::controls
{
// Multi-line text editor: caret navigation, line scrolling, IME composition.
class OMDemiurgeTextEdit : public OMDemiurgeRectNode
{
  public:
    explicit OMDemiurgeTextEdit(geom::OMFontSet *fontset = nullptr);
    ~OMDemiurgeTextEdit() override;

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    void update() override;

    auto processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processMouseMove(float x, float y) -> OMDemiurgeEventResult override;
    auto processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult override;
    auto processKeyDown(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult override;
    auto processTextInput(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult override;
    auto processTextEditing(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult override;
    auto processFocusGained() -> void override;
    auto processFocusLost() -> void override;

    void setText(const std::string &text);
    auto getText() const -> std::string;
    void setPlaceholder(const std::string &placeholder);
    void setMaxLength(int codepoints);
    void setOnChange(std::function<void(const std::string &)> handler);
    void setTextHeight(int height);
    void setColors(glm::vec4 background, glm::vec4 text, glm::vec4 caret);
    void setDisabled(bool value, bool animate = true);
    auto isDisabled() const -> bool
    {
        return disabled;
    }

    animation::OMAnimationValue<float> focusAmount;
    animation::OMAnimationValue<float> disabledAmount;
    animation::OMAnimationValue<glm::vec4> backgroundColor;
    animation::OMAnimationValue<glm::vec4> textColor;
    animation::OMAnimationValue<glm::vec4> caretColor;
    animation::OMAnimationValue<glm::vec4> placeholderColor;
    animation::OMAnimationValue<float> caretXAnimation;

  private:
    auto lineStartOf(size_t pos) const -> size_t;
    auto lineEndOf(size_t pos) const -> size_t;
    auto lineIndexOf(size_t pos) const -> int;
    auto lineCount() const -> int;
    auto caretFromPointer(float x, float y) -> size_t;
    auto columnXForPos(size_t pos) -> float;
    auto moveVertical(int direction) -> void;
    auto insertText(const std::string &value) -> bool;
    auto notifyChange() -> void;
    auto selectionBegin() const -> size_t
    {
        return std::min(caret, selectionAnchor);
    }
    auto selectionEnd() const -> size_t
    {
        return std::max(caret, selectionAnchor);
    }
    auto hasSelection() const -> bool
    {
        return selectionAnchor != caret;
    }
    auto deleteSelection() -> bool;

    std::vector<std::shared_ptr<OMDemiurgeNode>> lineNodes;
    std::vector<std::shared_ptr<OMDemiurgeNode>> selectionNodes;
    std::shared_ptr<OMDemiurgeNode> caretNode;
    size_t selectionAnchor = 0;
    bool selecting = false;

    geom::OMFontSet *fontset = nullptr;
    std::string text;
    std::string placeholder;
    std::string composing;
    size_t caret = 0;
    int maxLength = 4096;
    bool disabled = false;
    float scrollX = 0.0f;
    float scrollY = 0.0f;
    float lastScrollX = 0.0f;
    float lastScrollY = 0.0f;
    float lastCaretTarget = -1.0f;
    bool clipInitialized = false;
    std::chrono::steady_clock::time_point blinkStart = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastComposingAt = std::chrono::steady_clock::now();
    bool composingEnded = false;
    std::chrono::steady_clock::time_point composingEndedAt = std::chrono::steady_clock::now();

    std::function<void(const std::string &)> handleChange = [](const std::string &) {};

    mutable std::mutex stateMutex;
};
} // namespace openminecraft::renderer::common::demiurge::node::controls

#endif
