#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_inputbox.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/om_text_input_controller.hpp"

#include <SDL3/SDL_keycode.h>

#include <algorithm>
#include <cmath>

using namespace openminecraft::renderer::common::animation;

namespace openminecraft::renderer::common::demiurge::node::controls
{
static auto isContinuation(unsigned char c) -> bool
{
    return (c & 0xC0) == 0x80;
}

static auto prevCodepoint(const std::string &text, size_t index) -> size_t
{
    if (index == 0)
    {
        return 0;
    }

    --index;

    while (index > 0 && isContinuation(static_cast<unsigned char>(text[index])))
    {
        --index;
    }

    return index;
}

static auto nextCodepoint(const std::string &text, size_t index) -> size_t
{
    if (index >= text.size())
    {
        return text.size();
    }

    ++index;

    while (index < text.size() && isContinuation(static_cast<unsigned char>(text[index])))
    {
        ++index;
    }

    return index;
}

static auto codepointCount(const std::string &text) -> int
{
    int count = 0;

    for (size_t i = 0; i < text.size(); i = nextCodepoint(text, i))
    {
        ++count;
    }

    return count;
}

OMDemiurgeInputBox::OMDemiurgeInputBox(geom::OMFontSet *fontset)
    : focusAmount(0.0f), disabledAmount(0.0f), backgroundColor(glm::vec4(0.12f, 0.12f, 0.15f, 0.95f)),
      textColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)), caretColor(glm::vec4(0.0f, 0.831f, 1.0f, 1.0f)),
      placeholderColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.35f)), caretXAnimation(0.0f), fontset(fontset)
{
    setFocusable(true);

    stylesStorage.put("width", OMDemiurgeSize::pixels(220.0f));
    stylesStorage.put("height", OMDemiurgeSize::pixels(26.0f));
    stylesStorage.put("radius", glm::vec4(6.0f));
    stylesStorage.put("padding", OMDemiurgeEdgeInsets{3, 8, 3, 8});

    if (fontset != nullptr)
    {
        textNode = std::make_shared<OMDemiurgeTextSdfNode>(fontset);
        textNode->style({
            {"position", OMDemiurgePosition::Absolute},
            {"offsetX", OMDemiurgeSize::pixels(0.0f)},
            {"offsetY", OMDemiurgeSize::pixels(0.0f)},
        });
        this->mountDirect(textNode);
    }

    caretNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
        {"offsetY", OMDemiurgeSize::pixels(0.0f)},
        {"width", OMDemiurgeSize::pixels(1.0f)},
        {"height", OMDemiurgeSize::pixels(16.0f)},
    });

    this->mountDirect(caretNode);
}

OMDemiurgeInputBox::~OMDemiurgeInputBox()
{
    if (activeInput.lock().get() == this)
    {
        activeInput.reset();
        common::OMTextInputController::instance().setActive(false);
    }
}

auto OMDemiurgeInputBox::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    OMDemiurgeRectNode::submit(handler, depth);
}

auto OMDemiurgeInputBox::measure(const std::string &value) -> float
{
    if (fontset == nullptr)
    {
        return 0.0f;
    }

    const int textHeight = stylesStorage.get<int>("textheight", 14);
    auto shaped = fontset->shape(value);
    return fontset->bound(shaped).x * static_cast<float>(textHeight);
}

auto OMDemiurgeInputBox::caretFromX(float x) -> size_t
{
    if (fontset == nullptr || text.empty())
    {
        return 0;
    }

    auto shaped = fontset->shape(text);

    std::vector<size_t> boundaries;
    boundaries.push_back(0);

    for (size_t pos = 0; pos < text.size();)
    {
        pos = nextCodepoint(text, pos);
        boundaries.push_back(pos);
    }

    size_t best = 0;
    float bestDistance = std::abs(x);

    for (size_t i = 0; i < boundaries.size(); ++i)
    {
        float width = 0.0f;

        if (i < shaped.size())
        {
            width = shaped[i].position.x * static_cast<float>(stylesStorage.get<int>("textheight", 14));
        }
        else if (!shaped.empty())
        {
            const auto &last = shaped.back();
            width = (last.position.x + last.size.x) * static_cast<float>(stylesStorage.get<int>("textheight", 14));
        }

        const float distance = std::abs(x - width);

        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = boundaries[i];
        }
    }

    return best;
}

auto OMDemiurgeInputBox::insertText(const std::string &value) -> bool
{
    if (value.empty())
    {
        return false;
    }

    std::string filtered;

    for (char c : value)
    {
        if (static_cast<unsigned char>(c) >= 0x20 && c != 0x7F)
        {
            filtered.push_back(c);
        }
    }

    if (filtered.empty())
    {
        return false;
    }

    const int current = codepointCount(text);
    const int incoming = codepointCount(filtered);

    if (current + incoming > maxLength)
    {
        return false;
    }

    text.insert(caret, filtered);
    caret += filtered.size();
    blinkStart = std::chrono::steady_clock::now();
    return true;
}

auto OMDemiurgeInputBox::notifyChange() -> void
{
    handleChange(text);
}

void OMDemiurgeInputBox::update()
{
    const int textHeight = stylesStorage.get<int>("textheight", 14);
    const float focus = std::clamp(focusAmount.get(), 0.0f, 1.0f);
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);

    glm::vec4 background = backgroundColor.get();
    background = glm::vec4(glm::mix(glm::vec3(background), glm::vec3(0.16f, 0.42f, 0.55f), focus * 0.35f),
                           background.a);
    background = glm::vec4(glm::mix(glm::vec3(background), glm::vec3(0.30f), disabledValue * 0.75f),
                           background.a * (1.0f - disabledValue * 0.35f));

    this->style("color", (int)f4ToRgba(background))
        ->style("radius", stylesStorage.get<glm::vec4>("radius", glm::vec4(6.0f)));

    // The event bus dispatches on its own thread; copy the editing state under the
    // lock so a concurrent composition update cannot tear the strings mid-render
    // (which showed up as flickering / vanishing text while using an IME).
    std::string textState;
    std::string composingState;
    size_t caretState = 0;
    bool disabledState = false;
    std::chrono::steady_clock::time_point blinkStartState = blinkStart;
    float lastComposingActivity = 10.0f;

    {
        std::lock_guard<std::mutex> lock(stateMutex);

        textState = text;
        composingState = composing;
        caretState = caret;
        disabledState = disabled;
        blinkStartState = blinkStart;
        lastComposingActivity =
            std::chrono::duration<float>(std::chrono::steady_clock::now() - lastComposingAt).count();
    }

    // Composition preview is spliced in at the caret; text itself is only updated
    // when the IME commits (SDL_EVENT_TEXT_INPUT).
    const size_t caretClamped = std::min(caretState, textState.size());
    const std::string display = composingState.empty()
                                    ? textState
                                    : textState.substr(0, caretClamped) + composingState +
                                          textState.substr(caretClamped);
    const size_t displayCaret = composingState.empty() ? caretClamped : caretClamped + composingState.size();
    const bool composingActiveChanged = composingWasActive != !composingState.empty();
    const bool empty = display.empty();
    // Do not flash the placeholder while an IME composition is (or was just)
    // active; show an empty field instead.
    const bool suppressPlaceholder = empty && lastComposingActivity < 1.0f;

    const auto color = empty ? placeholderColor.get() : textColor.get();
    const glm::vec4 finalColor(glm::mix(glm::vec3(color), glm::vec3(0.55f), disabledValue * 0.75f), color.a);

    float clipWidth = stylesStorage.get<float>("width", 220.0f) - 16.0f;

    try
    {
        auto bound = this->fetchLayoutBound();
        clipWidth = std::max(0.0f, bound.width - 16.0f);
    }
    catch (std::bad_any_cast &)
    {
    }

    // Glyph advance positions, used for scrolling, culling and caret placement.
    // Must follow the composed display string (text + IME preview), otherwise the
    // culling slice indexes get out of sync and the composition disappears.
    std::vector<size_t> boundaries;
    boundaries.push_back(0);

    for (size_t pos = 0; pos < display.size();)
    {
        pos = nextCodepoint(display, pos);
        boundaries.push_back(pos);
    }

    std::vector<float> glyphX;
    float textWidth = 0.0f;
    std::vector<geom::OMFontSetShapeResult> shaped;

    if (fontset != nullptr && !display.empty())
    {
        shaped = fontset->shape(display);

        for (auto &g : shaped)
        {
            glyphX.push_back(g.position.x * static_cast<float>(textHeight));
        }

        const auto &last = shaped.back();
        textWidth = (last.position.x + last.size.x) * static_cast<float>(textHeight);
    }

    float caretX = 0.0f;

    if (!shaped.empty())
    {
        const int before = codepointCount(display.substr(0, displayCaret));

        if (before < static_cast<int>(glyphX.size()))
        {
            caretX = glyphX[before];
        }
        else
        {
            caretX = textWidth;
        }
    }

    const float maxScroll = std::max(0.0f, textWidth - clipWidth + 2.0f);
    static float scroll = 0.0f;

    if (caretX - scroll > clipWidth - 2.0f)
    {
        scroll = caretX - clipWidth + 2.0f;
    }

    if (caretX < scroll)
    {
        scroll = caretX;
    }

    scroll = std::clamp(scroll, 0.0f, maxScroll);
    lastScroll = scroll;

    // Cull glyphs outside the visible range instead of using a clip rect (the
    // depth-based clip mask hides unrelated elements of the whole UI layer).
    std::string visible = empty ? (suppressPlaceholder ? std::string() : placeholder) : display;
    float textOffset = 0.0f;

    if (!empty && !shaped.empty())
    {
        size_t first = 0;
        size_t last = shaped.size();

        for (size_t i = 0; i < shaped.size(); ++i)
        {
            const float right = glyphX[i] + shaped[i].size.x * static_cast<float>(textHeight);

            if (right >= scroll)
            {
                first = i;
                break;
            }
        }

        for (size_t i = shaped.size(); i-- > 0;)
        {
            if (glyphX[i] <= scroll + clipWidth)
            {
                last = i + 1;
                break;
            }
        }

        first = std::min(first, shaped.size() - 1);
        last = std::min(last, shaped.size());

        if (last <= first || first >= boundaries.size() - 1)
        {
            visible.clear();
        }
        else
        {
            const size_t begin = boundaries[first];
            const size_t end = boundaries[std::min(last, boundaries.size() - 1)];
            visible = display.substr(begin, end - begin);
        }

        textOffset = glyphX[std::min(first, glyphX.size() - 1)] - scroll;
    }

    // Center the visible ink vertically inside the content box.
    float contentHeight = 20.0f;

    try
    {
        auto bound = this->fetchLayoutBound();
        contentHeight = std::max(0.0f, bound.height - 6.0f);
    }
    catch (std::bad_any_cast &)
    {
    }

    // Center the font's line box, not the ink of the current string: using the ink
    // makes the whole text jump up/down as soon as a descender (j, p, ...) appears.
    float lineHeight = static_cast<float>(textHeight);

    if (fontset != nullptr)
    {
        auto reference = fontset->shape("A");
        const auto metrics = fontset->bound(reference);

        if (metrics.y > 0.0f)
        {
            lineHeight = metrics.y * static_cast<float>(textHeight);
        }
    }

    const float textOffsetY = std::max(0.0f, (contentHeight - lineHeight) * 0.5f);

    if (textNode)
    {
        textNode->style("text", visible)
            ->style("textheight", textHeight)
            ->style("color", (int)f4ToRgba(finalColor))
            ->style("offsetX", OMDemiurgeSize::pixels(textOffset))
            ->style("offsetY", OMDemiurgeSize::pixels(textOffsetY));
    }

    const auto now = std::chrono::steady_clock::now();
    const float elapsed = std::chrono::duration<float>(now - blinkStartState).count();

    // Smooth blink: eased pulse instead of a hard on/off toggle.
    const float phase = std::fmod(elapsed, 1.0f);
    const float pulse = 0.5f + 0.5f * std::cos(phase * 6.2831853f);
    const float blinkAlpha = 0.15f + 0.85f * std::pow(pulse, 0.7f);

    // While an IME composition is active the input method draws its own insertion
    // caret, so hide ours to avoid showing two carets at once.
    const bool caretVisible = !disabledState && focus > 0.5f && composingState.empty();

    // Slide the caret to its new position instead of teleporting; long jumps snap.
    const float caretTarget = caretX - scroll + 1.0f;

    if (composingActiveChanged)
    {
        caretXAnimation.setTo(caretTarget);
        lastCaretTarget = caretTarget;
        composingWasActive = !composingState.empty();
    }
    else if (lastCaretTarget < 0.0f)
    {
        caretXAnimation.setTo(caretTarget);
        lastCaretTarget = caretTarget;
    }
    else if (std::abs(caretTarget - lastCaretTarget) > 0.01f)
    {
        if (std::abs(caretTarget - lastCaretTarget) > 160.0f)
        {
            caretXAnimation.setTo(caretTarget);
        }
        else
        {
            caretXAnimation.animateTo(caretTarget, easeOutCubic<float>, 0.08f);
        }

        lastCaretTarget = caretTarget;
    }

    const auto caretBase = caretColor.get();
    const float caretHeight = std::max(2.0f, static_cast<float>(textHeight));
    caretNode->style("offsetX", OMDemiurgeSize::pixels(caretXAnimation.get()))
        ->style("offsetY", OMDemiurgeSize::pixels(textOffsetY + (lineHeight - caretHeight) * 0.5f))
        ->style("height", OMDemiurgeSize::pixels(caretHeight))
        ->style("width", OMDemiurgeSize::pixels(caretVisible ? 1.0f + 0.6f * pulse : 1.0f))
        ->style("color", (int)f4ToRgba(glm::vec4(glm::mix(glm::vec3(caretBase), glm::vec3(0.55f), disabledValue),
                                                  caretVisible ? caretBase.a * blinkAlpha : 0.0f)));

    // Keep the IME candidate window anchored to the caret. The window is the only
    // place that talks to SDL; here we just publish the caret rectangle.
    if (hasFocus() && !disabledState && fontset != nullptr)
    {
        try
        {
            auto caretBound = caretNode->fetchLayoutBound();
            const float width = std::max(1.0f, caretBound.width);
            const float height = std::max(1.0f, caretBound.height);

            if (std::abs(caretBound.x - lastAreaX) > 0.5f || std::abs(caretBound.y - lastAreaY) > 0.5f ||
                std::abs(width - lastAreaWidth) > 0.5f || std::abs(height - lastAreaHeight) > 0.5f)
            {
                lastAreaX = caretBound.x;
                lastAreaY = caretBound.y;
                lastAreaWidth = width;
                lastAreaHeight = height;
                common::OMTextInputController::instance().setArea(lastAreaX, lastAreaY, width, height);
            }
        }
        catch (std::bad_any_cast &)
        {
        }
    }

    OMDemiurgeRectNode::update();
}

auto OMDemiurgeInputBox::scrollForCaret() -> float
{
    return lastScroll;
}

auto OMDemiurgeInputBox::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled || fontset == nullptr)
    {
        return Ignored;
    }

    float clipX = x;

    try
    {
        auto bound = this->fetchLayoutBound();
        clipX = x - bound.x - 8.0f;
    }
    catch (std::bad_any_cast &)
    {
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        caret = caretFromX(clipX + scrollForCaret());
        blinkStart = std::chrono::steady_clock::now();
    }

    return Handled;
}

auto OMDemiurgeInputBox::processKeyDown(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    bool changed = false;
    bool submit = false;
    bool release = false;
    OMDemiurgeEventResult result = Handled;
    std::string submitted;

    {
        std::lock_guard<std::mutex> lock(stateMutex);

        if (!composing.empty())
        {
            if (event.keycode == SDLK_ESCAPE)
            {
                composing.clear();
                composingEnded = false;
            }
            else if (event.keycode != SDLK_RETURN && event.keycode != SDLK_KP_ENTER)
            {
                // Let the IME own editing keys while composing.
                result = Ignored;
            }
        }
        else
        {
            switch (event.keycode)
            {
            case SDLK_BACKSPACE: {
                if (caret > 0)
                {
                    const size_t previous = prevCodepoint(text, caret);
                    text.erase(previous, caret - previous);
                    caret = previous;
                    blinkStart = std::chrono::steady_clock::now();
                    changed = true;
                }

                break;
            }
            case SDLK_DELETE: {
                if (caret < text.size())
                {
                    const size_t next = nextCodepoint(text, caret);
                    text.erase(caret, next - caret);
                    blinkStart = std::chrono::steady_clock::now();
                    changed = true;
                }

                break;
            }
            case SDLK_LEFT:
                caret = prevCodepoint(text, caret);
                blinkStart = std::chrono::steady_clock::now();
                break;
            case SDLK_RIGHT:
                caret = nextCodepoint(text, caret);
                blinkStart = std::chrono::steady_clock::now();
                break;
            case SDLK_HOME:
                caret = 0;
                blinkStart = std::chrono::steady_clock::now();
                break;
            case SDLK_END:
                caret = text.size();
                blinkStart = std::chrono::steady_clock::now();
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                submitted = text;
                submit = true;
                break;
            case SDLK_ESCAPE:
                release = true;
                break;
            default:
                result = Ignored;
                break;
            }
        }
    }

    if (submit)
    {
        handleSubmit(submitted);
    }

    if (changed)
    {
        notifyChange();
    }

    if (release)
    {
        releaseFocus();
    }

    return result;
}

auto OMDemiurgeInputBox::processTextInput(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    bool changed = false;

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        composing.clear();
        composingEnded = false;
        changed = insertText(event.text);
    }

    if (changed)
    {
        notifyChange();
    }

    return Handled;
}

auto OMDemiurgeInputBox::processTextEditing(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    // SDL sends an empty TEXT_EDITING right before the committed TEXT_INPUT; clearing
    // the preview immediately made the field alternate between composition and
    // placeholder for a frame. Keep the last preview until the commit arrives
    // (TextInput clears it) or the composition is cancelled (Esc / focus loss).
    std::lock_guard<std::mutex> lock(stateMutex);

    if (event.text.empty())
    {
        // Might be the end of a commit (TextInput follows) or a cancellation.
        // Defer clearing for a few frames to avoid flashing the placeholder.
        composingEnded = true;
        composingEndedAt = std::chrono::steady_clock::now();

        return Handled;
    }

    composingEnded = false;
    composing = event.text;

    composingStart = event.compositionStart;
    composingLength = event.compositionLength;
    lastComposingAt = std::chrono::steady_clock::now();
    blinkStart = std::chrono::steady_clock::now();
    return Handled;
}

auto OMDemiurgeInputBox::processFocusGained() -> void
{
    activeInput = std::static_pointer_cast<OMDemiurgeInputBox>(shared_from_this());
    common::OMTextInputController::instance().setActive(true);
    focusAmount.animateTo(1.0f, easeOutCirc<float>, 0.15f);
    blinkStart = std::chrono::steady_clock::now();
}

auto OMDemiurgeInputBox::processFocusLost() -> void
{
    if (activeInput.lock().get() == this)
    {
        activeInput.reset();
        common::OMTextInputController::instance().setActive(false);
    }

    composing.clear();
    composingEnded = false;

    focusAmount.animateTo(0.0f, easeOutCirc<float>, 0.15f);
}

void OMDemiurgeInputBox::setText(const std::string &value)
{
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        text = value;
        caret = text.size();
        blinkStart = std::chrono::steady_clock::now();
    }

    notifyChange();
}

void OMDemiurgeInputBox::setPlaceholder(const std::string &value)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    placeholder = value;
}

void OMDemiurgeInputBox::setMaxLength(int codepoints)
{
    maxLength = std::max(1, codepoints);
}

void OMDemiurgeInputBox::setOnChange(std::function<void(const std::string &)> handler)
{
    handleChange = std::move(handler);
}

void OMDemiurgeInputBox::setOnSubmit(std::function<void(const std::string &)> handler)
{
    handleSubmit = std::move(handler);
}

void OMDemiurgeInputBox::setTextHeight(int height)
{
    stylesStorage.put("textheight", height);
}

void OMDemiurgeInputBox::setColors(glm::vec4 background, glm::vec4 text, glm::vec4 caret)
{
    backgroundColor.animateTo(background, easeOutCirc<float>, 0.18f);
    textColor.animateTo(text, easeOutCirc<float>, 0.18f);
    caretColor.animateTo(caret, easeOutCirc<float>, 0.18f);
}

void OMDemiurgeInputBox::setDisabled(bool value, bool animate)
{
    std::lock_guard<std::mutex> lock(stateMutex);

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
