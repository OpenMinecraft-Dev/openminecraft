#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_inputbox.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"

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
      placeholderColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.35f)), fontset(fontset)
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

auto OMDemiurgeInputBox::insertText(const std::string &value) -> void
{
    if (value.empty())
    {
        return;
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
        return;
    }

    const int current = codepointCount(text);
    const int incoming = codepointCount(filtered);

    if (current + incoming > maxLength)
    {
        return;
    }

    text.insert(caret, filtered);
    caret += filtered.size();
    blinkStart = std::chrono::steady_clock::now();
    notifyChange();
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

    const bool empty = text.empty();

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
    std::vector<size_t> boundaries;
    boundaries.push_back(0);

    for (size_t pos = 0; pos < text.size();)
    {
        pos = nextCodepoint(text, pos);
        boundaries.push_back(pos);
    }

    std::vector<float> glyphX;
    float textWidth = 0.0f;
    std::vector<geom::OMFontSetShapeResult> shaped;

    if (fontset != nullptr && !text.empty())
    {
        shaped = fontset->shape(text);

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
        const int before = codepointCount(text.substr(0, caret));

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
    std::string visible = empty ? placeholder : text;
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

        if (last <= first)
        {
            visible.clear();
        }
        else
        {
            visible = text.substr(boundaries[first], boundaries[last] - boundaries[first]);
        }

        textOffset = glyphX[first] - scroll;
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
    const float elapsed = std::chrono::duration<float>(now - blinkStart).count();
    const bool blink = std::fmod(elapsed, 1.0f) < 0.6f;
    const bool caretVisible = !disabled && focus > 0.5f && blink;

    const auto caretBase = caretColor.get();
    const float caretHeight = std::max(2.0f, static_cast<float>(textHeight));
    caretNode->style("offsetX", OMDemiurgeSize::pixels(caretX - scroll + 1.0f))
        ->style("offsetY", OMDemiurgeSize::pixels(textOffsetY + (lineHeight - caretHeight) * 0.5f))
        ->style("height", OMDemiurgeSize::pixels(caretHeight))
        ->style("color", (int)f4ToRgba(glm::vec4(glm::mix(glm::vec3(caretBase), glm::vec3(0.55f), disabledValue),
                                                  caretVisible ? caretBase.a : 0.0f)));

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

    caret = caretFromX(clipX + scrollForCaret());
    blinkStart = std::chrono::steady_clock::now();
    return Handled;
}

auto OMDemiurgeInputBox::processKeyDown(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    switch (event.keycode)
    {
    case SDLK_BACKSPACE: {
        if (caret == 0)
        {
            return Handled;
        }

        const size_t previous = prevCodepoint(text, caret);
        text.erase(previous, caret - previous);
        caret = previous;
        blinkStart = std::chrono::steady_clock::now();
        notifyChange();
        return Handled;
    }
    case SDLK_DELETE: {
        if (caret >= text.size())
        {
            return Handled;
        }

        const size_t next = nextCodepoint(text, caret);
        text.erase(caret, next - caret);
        blinkStart = std::chrono::steady_clock::now();
        notifyChange();
        return Handled;
    }
    case SDLK_LEFT:
        caret = prevCodepoint(text, caret);
        blinkStart = std::chrono::steady_clock::now();
        return Handled;
    case SDLK_RIGHT:
        caret = nextCodepoint(text, caret);
        blinkStart = std::chrono::steady_clock::now();
        return Handled;
    case SDLK_HOME:
        caret = 0;
        blinkStart = std::chrono::steady_clock::now();
        return Handled;
    case SDLK_END:
        caret = text.size();
        blinkStart = std::chrono::steady_clock::now();
        return Handled;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        handleSubmit(text);
        return Handled;
    case SDLK_ESCAPE:
        releaseFocus();
        return Handled;
    default:
        break;
    }

    return Ignored;
}

auto OMDemiurgeInputBox::processTextInput(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    insertText(event.text);
    return Handled;
}

auto OMDemiurgeInputBox::processFocusGained() -> void
{
    activeInput = std::static_pointer_cast<OMDemiurgeInputBox>(shared_from_this());
    focusAmount.animateTo(1.0f, easeOutCirc<float>, 0.15f);
    blinkStart = std::chrono::steady_clock::now();
}

auto OMDemiurgeInputBox::processFocusLost() -> void
{
    if (activeInput.lock().get() == this)
    {
        activeInput.reset();
    }

    focusAmount.animateTo(0.0f, easeOutCirc<float>, 0.15f);
}

void OMDemiurgeInputBox::setText(const std::string &value)
{
    text = value;
    caret = text.size();
    blinkStart = std::chrono::steady_clock::now();
    notifyChange();
}

void OMDemiurgeInputBox::setPlaceholder(const std::string &value)
{
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
