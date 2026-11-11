#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_textinput.hpp"
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

OMDemiurgeTextInput::OMDemiurgeTextInput(geom::OMFontSet *fontset)
    : focusAmount(0.0f), disabledAmount(0.0f), backgroundColor(glm::vec4(0.12f, 0.12f, 0.15f, 0.6f)),
      textColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)), caretColor(glm::vec4(0.0f, 0.831f, 1.0f, 1.0f)),
      placeholderColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.35f)), caretXAnimation(0.0f), fontset(fontset)
{
    setFocusable(true);

    stylesStorage.put("width", OMDemiurgeSize::pixels(220.0f));
    stylesStorage.put("height", OMDemiurgeSize::pixels(26.0f));
    stylesStorage.put("radius", glm::vec4(6.0f));
    stylesStorage.put("padding", OMDemiurgeEdgeInsets{3, 8, 3, 8});

    selectionNode = std::make_shared<OMDemiurgeRectNode>()->style({
        {"position", OMDemiurgePosition::Absolute},
        {"offsetX", OMDemiurgeSize::pixels(0.0f)},
        {"offsetY", OMDemiurgeSize::pixels(0.0f)},
        {"width", OMDemiurgeSize::pixels(0.0f)},
        {"height", OMDemiurgeSize::pixels(16.0f)},
        {"radius", glm::vec4(2.0f)},
    });
    this->mountDirect(selectionNode);

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

OMDemiurgeTextInput::~OMDemiurgeTextInput()
{
    if (activeInput.lock().get() == this)
    {
        activeInput.reset();
        common::OMTextInputController::instance().setActive(false);
        common::OMTextInputController::instance().setTextFocus(false);
    }
}

auto OMDemiurgeTextInput::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    textNode->setClipNode(shared_from_this());
    OMDemiurgeRectNode::submit(handler, depth);
}

auto OMDemiurgeTextInput::measure(const std::string &value) -> float
{
    if (fontset == nullptr)
    {
        return 0.0f;
    }

    const int textHeight = stylesStorage.get<int>("textheight", 14);
    auto shaped = fontset->shape(value);
    return fontset->bound(shaped).x * static_cast<float>(textHeight);
}

auto OMDemiurgeTextInput::caretFromX(float x) -> size_t
{
    if (fontset == nullptr || text.empty())
    {
        return 0;
    }

    std::vector<size_t> boundaries;
    boundaries.push_back(0);

    for (size_t pos = 0; pos < text.size();)
    {
        pos = nextCodepoint(text, pos);
        boundaries.push_back(pos);
    }

    size_t best = 0;
    float bestDistance = std::abs(x);

    const int textHeight = stylesStorage.get<int>("textheight", 14);

    for (size_t i = 0; i < boundaries.size(); ++i)
    {
        const float width =
            fontset->advance(text.substr(0, boundaries[i])) * static_cast<float>(textHeight);
        const float distance = std::abs(x - width);

        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = boundaries[i];
        }
    }

    return best;
}

auto OMDemiurgeTextInput::insertText(const std::string &value) -> bool
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

auto OMDemiurgeTextInput::deleteSelection() -> bool
{
    if (!hasSelection())
    {
        return false;
    }

    const size_t begin = selectionBegin();
    const size_t end = selectionEnd();
    text.erase(begin, end - begin);
    caret = begin;
    selectionAnchor = begin;
    blinkStart = std::chrono::steady_clock::now();
    return true;
}

auto OMDemiurgeTextInput::displayIndex(size_t textIndex) const -> size_t
{
    return composing.empty() || textIndex <= caret ? textIndex : textIndex + composing.size();
}

auto OMDemiurgeTextInput::notifyChange() -> void
{
    handleChange(text);
}

void OMDemiurgeTextInput::update()
{
    const int textHeight = stylesStorage.get<int>("textheight", 14);
    const float focus = std::clamp(focusAmount.get(), 0.0f, 1.0f);
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);

    glm::vec4 background = backgroundColor.get();
    background =
        glm::vec4(glm::mix(glm::vec3(background), glm::vec3(0.16f, 0.42f, 0.55f), focus * 0.35f), background.a);
    background = glm::vec4(glm::mix(glm::vec3(background), glm::vec3(0.30f), disabledValue * 0.75f),
                           background.a * (1.0f - disabledValue * 0.35f));

    this->style("_internal.color", (int)f4ToRgba(background))
        ->style("radius", stylesStorage.get<glm::vec4>("radius", glm::vec4(6.0f)));

    // The event bus dispatches on its own thread; copy the editing state under the
    // lock so a concurrent composition update cannot tear the strings mid-render
    // (which showed up as flickering / vanishing text while using an IME).
    std::string textState;
    std::string composingState;
    size_t caretState = 0;
    size_t anchorState = 0;
    bool disabledState = false;
    std::chrono::steady_clock::time_point blinkStartState = blinkStart;
    float lastComposingActivity = 10.0f;

    {
        std::lock_guard<std::mutex> lock(stateMutex);

        textState = text;
        composingState = composing;
        caretState = caret;
        anchorState = selectionAnchor;
        disabledState = disabled;
        blinkStartState = blinkStart;
        lastComposingActivity =
            std::chrono::duration<float>(std::chrono::steady_clock::now() - lastComposingAt).count();
    }

    // Composition preview is spliced in at the caret; text itself is only updated
    // when the IME commits (SDL_EVENT_TEXT_INPUT).
    const size_t caretClamped = std::min(caretState, textState.size());
    const std::string display =
        composingState.empty() ? textState
                               : textState.substr(0, caretClamped) + composingState + textState.substr(caretClamped);
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

        textWidth = fontset->advance(display) * static_cast<float>(textHeight);
    }

    // Use pen advances so trailing spaces move the caret immediately.
    float caretX = fontset != nullptr
                       ? fontset->advance(display.substr(0, displayCaret)) * static_cast<float>(textHeight)
                       : 0.0f;

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

    {
        const size_t selBegin = std::min(caretState, anchorState);
        const size_t selEnd = std::max(caretState, anchorState);
        const bool showSelection = !disabledState && selBegin != selEnd && composingState.empty() && fontset != nullptr;

        if (showSelection)
        {
            const auto mapDisplay = [&](size_t index) -> size_t {
                return composingState.empty() || index <= caretState ? index : index + composingState.size();
            };

            const size_t displayBegin = std::min(mapDisplay(selBegin), display.size());
            const size_t displayEnd = std::min(mapDisplay(selEnd), display.size());
            float x0 = fontset->advance(display.substr(0, displayBegin)) * static_cast<float>(textHeight);
            float x1 = fontset->advance(display.substr(0, displayEnd)) * static_cast<float>(textHeight);
            x0 = std::clamp(x0, scroll, scroll + clipWidth);
            x1 = std::clamp(x1, scroll, scroll + clipWidth);

            selectionNode->style("offsetX", OMDemiurgeSize::pixels(x0 - scroll))
                ->style("offsetY", OMDemiurgeSize::pixels(textOffsetY + (lineHeight - textHeight) * 0.5f))
                ->style("width", OMDemiurgeSize::pixels(std::max(0.0f, x1 - x0)))
                ->style("height", OMDemiurgeSize::pixels(static_cast<float>(textHeight)))
                ->style("color", (int)f4ToRgba(glm::vec4(0.0f, 0.831f, 1.0f, 0.32f)));
        }
        else
        {
            selectionNode->style("width", OMDemiurgeSize::pixels(0.0f))
                ->style("height", OMDemiurgeSize::pixels(0.0f))
                ->style("color", (int)0);
        }
    }


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

auto OMDemiurgeTextInput::scrollForCaret() -> float
{
    return lastScroll;
}

auto OMDemiurgeTextInput::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
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
        selectionAnchor = caret;
        selecting = true;
        blinkStart = std::chrono::steady_clock::now();
    }

    captureMouse();
    return Handled;
}

auto OMDemiurgeTextInput::processMouseMove(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled || fontset == nullptr || !selecting)
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

auto OMDemiurgeTextInput::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    selecting = false;
    return Handled;
}

auto OMDemiurgeTextInput::processKeyDown(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
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
    const bool shift = (event.modifier & SDL_KMOD_SHIFT) != 0;
    const bool ctrl = (event.modifier & SDL_KMOD_CTRL) != 0;

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
                if (deleteSelection())
                {
                    changed = true;
                }
                else if (caret > 0)
                {
                    const size_t previous = prevCodepoint(text, caret);
                    text.erase(previous, caret - previous);
                    caret = previous;
                    selectionAnchor = caret;
                    blinkStart = std::chrono::steady_clock::now();
                    changed = true;
                }

                break;
            }
            case SDLK_DELETE: {
                if (deleteSelection())
                {
                    changed = true;
                }
                else if (caret < text.size())
                {
                    const size_t next = nextCodepoint(text, caret);
                    text.erase(caret, next - caret);
                    blinkStart = std::chrono::steady_clock::now();
                    changed = true;
                }

                break;
            }
            case SDLK_LEFT:
                if (!shift && hasSelection())
                {
                    caret = selectionBegin();
                    selectionAnchor = caret;
                }
                else
                {
                    if (shift && !hasSelection())
                    {
                        selectionAnchor = caret;
                    }

                    caret = prevCodepoint(text, caret);
                }

                if (!shift)
                {
                    selectionAnchor = caret;
                }

                blinkStart = std::chrono::steady_clock::now();
                break;
            case SDLK_RIGHT:
                if (!shift && hasSelection())
                {
                    caret = selectionEnd();
                    selectionAnchor = caret;
                }
                else
                {
                    if (shift && !hasSelection())
                    {
                        selectionAnchor = caret;
                    }

                    caret = nextCodepoint(text, caret);
                }

                if (!shift)
                {
                    selectionAnchor = caret;
                }

                blinkStart = std::chrono::steady_clock::now();
                break;
            case SDLK_HOME:
                if (shift && !hasSelection())
                {
                    selectionAnchor = caret;
                }

                caret = 0;

                if (!shift)
                {
                    selectionAnchor = caret;
                }

                blinkStart = std::chrono::steady_clock::now();
                break;
            case SDLK_END:
                if (shift && !hasSelection())
                {
                    selectionAnchor = caret;
                }

                caret = text.size();

                if (!shift)
                {
                    selectionAnchor = caret;
                }

                blinkStart = std::chrono::steady_clock::now();
                break;
            case SDLK_A:
                if (ctrl)
                {
                    selectionAnchor = 0;
                    caret = text.size();
                    blinkStart = std::chrono::steady_clock::now();
                }
                else
                {
                    result = Ignored;
                }

                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                submitted = text;
                submit = true;
                break;
            case SDLK_ESCAPE:
                if (hasSelection())
                {
                    selectionAnchor = caret;
                }
                else
                {
                    release = true;
                }

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

auto OMDemiurgeTextInput::processTextInput(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
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

        const bool removed = deleteSelection();
        const bool inserted = insertText(event.text);
        selectionAnchor = caret;
        changed = removed || inserted;
    }

    if (changed)
    {
        notifyChange();
    }

    return Handled;
}

auto OMDemiurgeTextInput::processTextEditing(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    // SDL sends an empty TEXT_EDITING right before the committed TEXT_INPUT; clearing
    // the preview immediately made the field alternate between composition and
    // placeholder for a frame. Keep the last preview until the commit arrives
    // (TextInput clears it) or the composition is cancelled (Esc / focus loss).
    bool changed = false;

    {
        std::lock_guard<std::mutex> lock(stateMutex);

        if (event.text.empty())
        {
            // Might be the end of a commit (TextInput follows) or a cancellation.
            // Defer clearing for a few frames to avoid flashing the placeholder.
            composingEnded = true;
            composingEndedAt = std::chrono::steady_clock::now();

            return Handled;
        }

        changed = deleteSelection();
        composingEnded = false;
        composing = event.text;

        composingStart = event.compositionStart;
        composingLength = event.compositionLength;
        lastComposingAt = std::chrono::steady_clock::now();
        blinkStart = std::chrono::steady_clock::now();
    }

    if (changed)
    {
        notifyChange();
    }

    return Handled;
}

auto OMDemiurgeTextInput::processFocusGained() -> void
{
    activeInput = std::static_pointer_cast<OMDemiurgeTextInput>(shared_from_this());
    common::OMTextInputController::instance().setActive(true);
    common::OMTextInputController::instance().setTextFocus(true);
    focusAmount.animateTo(1.0f, easeOutCirc<float>, 0.15f);
    blinkStart = std::chrono::steady_clock::now();
}

auto OMDemiurgeTextInput::processFocusLost() -> void
{
    if (activeInput.lock().get() == this)
    {
        activeInput.reset();
        common::OMTextInputController::instance().setActive(false);
    }

    common::OMTextInputController::instance().setTextFocus(false);

    composing.clear();
    composingEnded = false;
    selectionAnchor = caret;
    selecting = false;

    focusAmount.animateTo(0.0f, easeOutCirc<float>, 0.15f);
}

void OMDemiurgeTextInput::setText(const std::string &value)
{
    {
        std::lock_guard<std::mutex> lock(stateMutex);
        text = value;
        caret = text.size();
        selectionAnchor = caret;
        selecting = false;
        blinkStart = std::chrono::steady_clock::now();
    }

    notifyChange();
}

void OMDemiurgeTextInput::setPlaceholder(const std::string &value)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    placeholder = value;
}

void OMDemiurgeTextInput::setMaxLength(int codepoints)
{
    maxLength = std::max(1, codepoints);
}

void OMDemiurgeTextInput::setOnChange(std::function<void(const std::string &)> handler)
{
    handleChange = std::move(handler);
}

void OMDemiurgeTextInput::setOnSubmit(std::function<void(const std::string &)> handler)
{
    handleSubmit = std::move(handler);
}

void OMDemiurgeTextInput::setTextHeight(int height)
{
    stylesStorage.put("textheight", height);
}

void OMDemiurgeTextInput::setColors(glm::vec4 background, glm::vec4 text, glm::vec4 caret)
{
    backgroundColor.animateTo(background, easeOutCirc<float>, 0.18f);
    textColor.animateTo(text, easeOutCirc<float>, 0.18f);
    caretColor.animateTo(caret, easeOutCirc<float>, 0.18f);
}

void OMDemiurgeTextInput::setDisabled(bool value, bool animate)
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
