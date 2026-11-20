#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_textedit.hpp"
#include "openminecraft/renderer/common/animation/om_animation_easing.hpp"
#include "openminecraft/renderer/common/demiurge/node/controls/om_demiurge_label.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_event.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/om_text_input_controller.hpp"

#include <SDL3/SDL_keycode.h>

#include <algorithm>
#include <cmath>
#include <limits>

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

static constexpr int LinePoolSize = 32;

OMDemiurgeTextEdit::OMDemiurgeTextEdit(geom::OMFontSet *fontset)
    : focusAmount(0.0f), disabledAmount(0.0f), backgroundColor(glm::vec4(0.12f, 0.12f, 0.15f, 0.6f)),
      textColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)), caretColor(glm::vec4(0.0f, 0.831f, 1.0f, 1.0f)),
      placeholderColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.35f)), caretXAnimation(0.0f), fontset(fontset)
{
    setFocusable(true);

    stylesStorage.put("width", OMDemiurgeSize::pixels(260.0f));
    stylesStorage.put("height", OMDemiurgeSize::pixels(120.0f));
    stylesStorage.put("radius", glm::vec4(6.0f));
    stylesStorage.put("padding", OMDemiurgeEdgeInsets{4, 8, 4, 8});

    if (fontset != nullptr)
    {
        placeholderNode = std::make_shared<OMDemiurgeLabel>(fontset);
        placeholderNode->style({
            {"position", OMDemiurgePosition::Absolute},
            {"offsetX", OMDemiurgeSize::pixels(0.0f)},
            {"offsetY", OMDemiurgeSize::pixels(0.0f)},
        });
        this->mountDirect(placeholderNode);
    }

    for (int i = 0; i < LinePoolSize; ++i)
    {
        auto node = std::make_shared<OMDemiurgeRectNode>()->style({
            {"position", OMDemiurgePosition::Absolute},
            {"offsetX", OMDemiurgeSize::pixels(0.0f)},
            {"offsetY", OMDemiurgeSize::pixels(0.0f)},
            {"width", OMDemiurgeSize::pixels(0.0f)},
            {"height", OMDemiurgeSize::pixels(16.0f)},
            {"radius", glm::vec4(2.0f)},
        });
        this->mountDirect(node);
        selectionNodes.push_back(node);
    }

    if (fontset != nullptr)
    {
        for (int i = 0; i < LinePoolSize; ++i)
        {
            auto node = std::make_shared<OMDemiurgeLabel>(fontset);
            node->style({
                {"position", OMDemiurgePosition::Absolute},
                {"offsetX", OMDemiurgeSize::pixels(0.0f)},
                {"offsetY", OMDemiurgeSize::pixels(0.0f)},
            });
            this->mountDirect(node);
            lineNodes.push_back(node);
        }
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

OMDemiurgeTextEdit::~OMDemiurgeTextEdit() = default;

auto OMDemiurgeTextEdit::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    applyInternalClips();
    OMDemiurgeRectNode::submit(handler, depth);
}

auto OMDemiurgeTextEdit::applyInternalClips() -> void
{
    // An enclosing pane assigns its clip to the whole subtree; keep the editor's
    // own lines/selection/caret clipped to the editor (pane ∧ editor).
    auto self = std::static_pointer_cast<OMDemiurgeTextEdit>(shared_from_this());

    if (placeholderNode)
    {
        placeholderNode->setClipNode(self);
    }

    for (auto &node : lineNodes)
    {
        node->setClipNode(self);
    }

    for (auto &node : selectionNodes)
    {
        node->setClipNode(self);
    }

    caretNode->setClipNode(self);
}

auto OMDemiurgeTextEdit::lineStartOf(size_t pos) const -> size_t
{
    if (pos == 0)
    {
        return 0;
    }

    const size_t found = text.rfind('\n', pos - 1);
    return found == std::string::npos ? 0 : found + 1;
}

auto OMDemiurgeTextEdit::lineEndOf(size_t pos) const -> size_t
{
    const size_t found = text.find('\n', pos);
    return found == std::string::npos ? text.size() : found;
}

auto OMDemiurgeTextEdit::lineIndexOf(size_t pos) const -> int
{
    int line = 0;

    for (size_t i = 0; i < pos && i < text.size(); ++i)
    {
        if (text[i] == '\n')
        {
            ++line;
        }
    }

    return line;
}

auto OMDemiurgeTextEdit::lineCount() const -> int
{
    int count = 1;

    for (char c : text)
    {
        if (c == '\n')
        {
            ++count;
        }
    }

    return count;
}

auto OMDemiurgeTextEdit::columnXForPos(size_t pos) -> float
{
    // Caller must hold stateMutex.
    if (fontset == nullptr)
    {
        return 0.0f;
    }

    const size_t start = lineStartOf(pos);
    const std::string prefix = text.substr(start, std::min(pos, text.size()) - std::min(start, text.size()));

    const int textHeight = stylesStorage.get<int>("textHeight", 14);
    return fontset->advance(prefix) * static_cast<float>(textHeight);
}

auto OMDemiurgeTextEdit::moveVertical(int direction) -> void
{
    // Caller must hold stateMutex.
    if (fontset == nullptr)
    {
        return;
    }

    const int textHeight = stylesStorage.get<int>("textHeight", 14);
    const int currentLine = lineIndexOf(caret);
    const int targetLine = std::clamp(currentLine + direction, 0, lineCount() - 1);

    if (targetLine == currentLine)
    {
        return;
    }

    const size_t currentStart = lineStartOf(caret);
    const float targetX =
        fontset->advance(text.substr(currentStart, caret - currentStart)) * static_cast<float>(textHeight);

    size_t start = 0;
    int line = 0;

    while (line < targetLine)
    {
        const size_t nl = text.find('\n', start);
        start = nl + 1;
        ++line;
    }

    const size_t end = lineEndOf(start);
    size_t best = start;
    float bestDistance = std::abs(targetX);

    for (size_t pos = start; pos <= end; pos = nextCodepoint(text, pos))
    {
        const float x = fontset->advance(text.substr(start, pos - start)) * static_cast<float>(textHeight);
        const float distance = std::abs(targetX - x);

        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = pos;
        }

        if (pos == end)
        {
            break;
        }
    }

    caret = best;
    blinkStart = std::chrono::steady_clock::now();
}

auto OMDemiurgeTextEdit::caretFromPointer(float x, float y) -> size_t
{
    // Caller must hold stateMutex.
    if (fontset == nullptr)
    {
        return 0;
    }

    const int textHeight = stylesStorage.get<int>("textHeight", 14);
    const float lineHeight = fontset->bound(fontset->shape("A")).y * static_cast<float>(textHeight) * 1.25f;
    const int line = std::clamp(static_cast<int>(std::floor(y / lineHeight)), 0, lineCount() - 1);

    size_t start = 0;
    int current = 0;

    while (current < line)
    {
        const size_t nl = text.find('\n', start);
        start = nl + 1;
        ++current;
    }

    const size_t end = lineEndOf(start);
    size_t best = start;
    float bestDistance = std::abs(x);

    for (size_t pos = start; pos <= end; pos = nextCodepoint(text, pos))
    {
        const float posX = fontset->advance(text.substr(start, pos - start)) * static_cast<float>(textHeight);
        const float distance = std::abs(x - posX);

        if (distance < bestDistance)
        {
            bestDistance = distance;
            best = pos;
        }

        if (pos == end)
        {
            break;
        }
    }

    return best;
}

auto OMDemiurgeTextEdit::insertText(const std::string &value) -> bool
{
    std::string filtered;

    for (char c : value)
    {
        if (c == '\n' || c == '\t' || static_cast<unsigned char>(c) >= 0x20)
        {
            filtered.push_back(c);
        }
    }

    if (filtered.empty())
    {
        return false;
    }

    if (codepointCount(text) + codepointCount(filtered) > maxLength)
    {
        return false;
    }

    text.insert(caret, filtered);
    caret += filtered.size();
    blinkStart = std::chrono::steady_clock::now();
    return true;
}

auto OMDemiurgeTextEdit::deleteSelection() -> bool
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

auto OMDemiurgeTextEdit::notifyChange() -> void
{
    handleChange(text);
}

void OMDemiurgeTextEdit::update()
{
    const int textHeight = stylesStorage.get<int>("textHeight", 14);
    const float focus = std::clamp(focusAmount.get(), 0.0f, 1.0f);
    const float disabledValue = std::clamp(disabledAmount.get(), 0.0f, 1.0f);

    std::string textState;
    std::string composingState;
    size_t caretState = 0;
    size_t anchorState = 0;
    bool disabledState = false;
    std::chrono::steady_clock::time_point blinkState = blinkStart;

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        textState = text;
        composingState = composing;
        caretState = caret;
        anchorState = selectionAnchor;
        disabledState = disabled;
        blinkState = blinkStart;
    }

    glm::vec4 background = backgroundColor.get();
    background =
        glm::vec4(glm::mix(glm::vec3(background), glm::vec3(0.16f, 0.42f, 0.55f), focus * 0.35f), background.a);
    background = glm::vec4(glm::mix(glm::vec3(background), glm::vec3(0.30f), disabledValue * 0.75f),
                           background.a * (1.0f - disabledValue * 0.35f));

    this->style("internal.color", (int)f4ToRgba(background))
        ->style("radius", stylesStorage.get<glm::vec4>("radius", glm::vec4(6.0f)));

    float contentWidth = stylesStorage.get<float>("width", 260.0f) - 16.0f;
    float contentHeight = stylesStorage.get<float>("height", 120.0f) - 8.0f;

    try
    {
        auto bound = this->fetchLayoutBound();
        contentWidth = std::max(8.0f, bound.width - 16.0f);
        contentHeight = std::max(8.0f, bound.height - 8.0f);
    }
    catch (std::bad_any_cast &)
    {
    }

    float lineHeight = static_cast<float>(textHeight) * 1.25f;

    if (fontset != nullptr)
    {
        const auto metrics = fontset->bound(fontset->shape("A"));

        if (metrics.y > 0.0f)
        {
            lineHeight = metrics.y * static_cast<float>(textHeight) * 1.25f;
        }
    }

    lineHeight = stylesStorage.get<float>("textLineHeight", lineHeight);

    const size_t caretClamped = std::min(caretState, textState.size());
    const std::string display =
        composingState.empty() ? textState
                               : textState.substr(0, caretClamped) + composingState + textState.substr(caretClamped);
    const size_t displayCaret = composingState.empty() ? caretClamped : caretClamped + composingState.size();

    // Split display into lines and measure the caret position.
    std::vector<std::pair<size_t, size_t>> lines;
    {
        size_t start = 0;

        while (true)
        {
            const size_t pos = display.find('\n', start);
            lines.emplace_back(start, pos == std::string::npos ? display.size() : pos);

            if (pos == std::string::npos)
            {
                break;
            }

            start = pos + 1;
        }
    }

    int caretLine = 0;
    size_t caretLineStart = 0;

    for (size_t i = 0; i < display.size() && i < displayCaret; ++i)
    {
        if (display[i] == '\n')
        {
            ++caretLine;
            caretLineStart = i + 1;
        }
    }

    const size_t caretLineEnd = lines[std::min<size_t>(caretLine, lines.size() - 1)].second;

    auto lineX = [&](const std::string &line, size_t prefixBytes) -> float {
        if (fontset == nullptr)
        {
            return 0.0f;
        }

        return fontset->advance(line.substr(0, prefixBytes)) * static_cast<float>(textHeight);
    };

    const std::string caretLineText = display.substr(caretLineStart, std::min(caretLineEnd, display.size()) -
                                                                         std::min(caretLineStart, display.size()));
    const float caretX = lineX(caretLineText, std::min(displayCaret, display.size()) - caretLineStart);
    const float caretY = static_cast<float>(caretLine) * lineHeight;

    // Ensure the caret stays inside the viewport.
    if (caretX - scrollX > contentWidth - 2.0f)
    {
        scrollX = caretX - contentWidth + 2.0f;
    }

    if (caretX < scrollX)
    {
        scrollX = caretX;
    }

    if (caretY - scrollY + lineHeight > contentHeight)
    {
        scrollY = caretY + lineHeight - contentHeight;
    }

    if (caretY < scrollY)
    {
        scrollY = caretY;
    }

    float maxLineWidth = 0.0f;

    for (auto &line : lines)
    {
        if (fontset != nullptr)
        {
            maxLineWidth =
                std::max(maxLineWidth, fontset->advance(display.substr(line.first, line.second - line.first)) *
                                           static_cast<float>(textHeight));
        }
    }

    const float maxScrollX = std::max(0.0f, maxLineWidth - contentWidth + 2.0f);
    const float totalHeight = static_cast<float>(lines.size()) * lineHeight;
    const float maxScrollY = std::max(0.0f, totalHeight - contentHeight);

    scrollX = std::clamp(scrollX, 0.0f, maxScrollX);
    scrollY = std::clamp(scrollY, 0.0f, maxScrollY);
    lastScrollX = scrollX;
    lastScrollY = scrollY;

    const int firstVisible = std::max(0, static_cast<int>(std::floor(scrollY / lineHeight)) - 1);
    const int lastVisible = std::min(static_cast<int>(lines.size()) - 1,
                                     static_cast<int>(std::ceil((scrollY + contentHeight) / lineHeight)));

    // Selection highlight (behind the text, one rect per visible line).
    {
        const size_t selBegin = std::min(caretState, anchorState);
        const size_t selEnd = std::max(caretState, anchorState);
        const bool showSelection = !disabledState && selBegin != selEnd && composingState.empty() && fontset != nullptr;

        int selectionUsed = 0;

        if (showSelection)
        {
            for (int lineIndex = firstVisible; lineIndex <= lastVisible; ++lineIndex)
            {
                if (lineIndex < 0 || lineIndex >= static_cast<int>(lines.size()) ||
                    selectionUsed >= static_cast<int>(selectionNodes.size()))
                {
                    continue;
                }

                const auto &range = lines[lineIndex];
                const size_t lineStart = range.first;
                const size_t lineEnd = range.second;

                if (selEnd <= lineStart || selBegin > lineEnd)
                {
                    continue;
                }

                const size_t beginInLine = std::clamp(selBegin, lineStart, lineEnd) - lineStart;
                const size_t endInLine = std::clamp(selEnd, lineStart, lineEnd) - lineStart;
                const std::string lineText = display.substr(lineStart, lineEnd - lineStart);
                const int textHeightLocal = stylesStorage.get<int>("textHeight", 14);
                float x0 = fontset->advance(lineText.substr(0, beginInLine)) * static_cast<float>(textHeightLocal);
                float x1 = fontset->advance(lineText.substr(0, endInLine)) * static_cast<float>(textHeightLocal);

                x0 = std::clamp(x0, scrollX, scrollX + contentWidth);
                x1 = std::clamp(x1, scrollX, scrollX + contentWidth);

                if (x1 <= x0 && lineStart == lineEnd)
                {
                    x1 = x0 + 2.0f;
                }

                auto node = selectionNodes[selectionUsed++];
                node->style("offsetX", OMDemiurgeSize::pixels(x0 - scrollX))
                    ->style("offsetY", OMDemiurgeSize::pixels(static_cast<float>(lineIndex) * lineHeight - scrollY))
                    ->style("width", OMDemiurgeSize::pixels(std::max(0.0f, x1 - x0)))
                    ->style("height", OMDemiurgeSize::pixels(lineHeight))
                    ->style("color", (int)f4ToRgba(glm::vec4(0.0f, 0.831f, 1.0f, 0.32f)));
            }
        }

        for (int i = selectionUsed; i < static_cast<int>(selectionNodes.size()); ++i)
        {
            selectionNodes[i]
                ->style("width", OMDemiurgeSize::pixels(0.0f))
                ->style("height", OMDemiurgeSize::pixels(0.0f))
                ->style("color", (int)0);
        }
    }

    // Render only the visible lines / glyph slices.
    const bool empty = display.empty();
    const auto textBase = textColor.get();
    const glm::vec4 finalColor(glm::mix(glm::vec3(textBase), glm::vec3(0.55f), disabledValue * 0.75f), textBase.a);
    const auto placeholderBase = placeholderColor.get();
    const glm::vec4 placeholderFinalColor(glm::mix(glm::vec3(placeholderBase), glm::vec3(0.55f), disabledValue * 0.75f),
                                          placeholderBase.a);

    if (placeholderNode)
    {
        placeholderNode->style("text", empty ? placeholder : std::string())
            ->style("textHeight", textHeight)
            ->style("color", (int)f4ToRgba(placeholderFinalColor))
            ->style("offsetX", OMDemiurgeSize::pixels(0.0f))
            ->style("offsetY", OMDemiurgeSize::pixels(0.0f));
    }

    for (int i = 0; i < static_cast<int>(lineNodes.size()); ++i)
    {
        auto node = lineNodes[i];
        const int lineIndex = firstVisible + i;

        if (lineIndex > lastVisible || lineIndex >= static_cast<int>(lines.size()))
        {
            node->style("text", "");
            continue;
        }

        const auto &range = lines[lineIndex];
        const std::string lineText = display.substr(range.first, range.second - range.first);

        std::string visible = lineText;
        float offsetX = -scrollX;

        if (fontset != nullptr && !lineText.empty())
        {
            auto shaped = fontset->shape(lineText);
            std::vector<float> glyphX;
            glyphX.reserve(shaped.size());

            for (auto &g : shaped)
            {
                glyphX.push_back(g.position.x * static_cast<float>(textHeight));
            }

            size_t first = 0;
            size_t last = shaped.size();

            for (size_t g = 0; g < shaped.size(); ++g)
            {
                const float right = glyphX[g] + shaped[g].size.x * static_cast<float>(textHeight);

                if (right >= scrollX)
                {
                    first = g;
                    break;
                }
            }

            for (size_t g = shaped.size(); g-- > 0;)
            {
                if (glyphX[g] <= scrollX + contentWidth)
                {
                    last = g + 1;
                    break;
                }
            }

            first = std::min(first, shaped.size() - 1);
            last = std::min(last, shaped.size());

            std::vector<size_t> boundaries;
            boundaries.push_back(0);

            for (size_t pos = 0; pos < lineText.size();)
            {
                pos = nextCodepoint(lineText, pos);
                boundaries.push_back(pos);
            }

            if (last > first)
            {
                const size_t begin = boundaries[std::min(first, boundaries.size() - 1)];
                const size_t end = boundaries[std::min(last, boundaries.size() - 1)];
                visible = lineText.substr(begin, end - begin);
                offsetX = glyphX[std::min(first, glyphX.size() - 1)] - scrollX;
            }
            else
            {
                visible.clear();
            }
        }

        node->style("text", visible)
            ->style("textHeight", textHeight)
            ->style("color", (int)f4ToRgba(finalColor))
            ->style("offsetX", OMDemiurgeSize::pixels(offsetX))
            ->style("offsetY", OMDemiurgeSize::pixels(static_cast<float>(lineIndex) * lineHeight - scrollY));
    }

    // Caret: smooth blink + visible only while focused / not composing.
    const bool caretVisible = !disabledState && focus > 0.5f && composingState.empty();
    const auto now = std::chrono::steady_clock::now();
    const float elapsed = std::chrono::duration<float>(now - blinkState).count();
    const float phase = std::fmod(elapsed, 1.0f);
    const float pulse = 0.5f + 0.5f * std::cos(phase * 6.2831853f);
    const float blinkAlpha = 0.15f + 0.85f * std::pow(pulse, 0.7f);

    const float caretTarget = caretX - scrollX;

    if (lastCaretTarget < 0.0f)
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
    caretNode->style("offsetX", OMDemiurgeSize::pixels(caretXAnimation.get()))
        ->style("offsetY", OMDemiurgeSize::pixels(caretY - scrollY))
        ->style("height", OMDemiurgeSize::pixels(static_cast<float>(textHeight)))
        ->style("width", OMDemiurgeSize::pixels(1.0f))
        ->style("color", (int)f4ToRgba(glm::vec4(glm::mix(glm::vec3(caretBase), glm::vec3(0.55f), disabledValue),
                                                 caretVisible ? caretBase.a * blinkAlpha : 0.0f)));

    if (hasFocus() && !disabledState && fontset != nullptr)
    {
        try
        {
            auto caretBound = caretNode->fetchLayoutBound();
            const float width = std::max(1.0f, caretBound.width);
            const float height = std::max(1.0f, caretBound.height);

            common::OMTextInputController::instance().setArea(caretBound.x, caretBound.y, width, height);
        }
        catch (std::bad_any_cast &)
        {
        }
    }

    OMDemiurgeRectNode::update();
}

auto OMDemiurgeTextEdit::processMouseDown(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    if (disabled || fontset == nullptr)
    {
        return Ignored;
    }

    float localX = x;
    float localY = y;

    try
    {
        auto bound = this->fetchLayoutBound();
        localX = x - bound.x - 8.0f + scrollX;
        localY = y - bound.y - 4.0f + scrollY;
    }
    catch (std::bad_any_cast &)
    {
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        caret = caretFromPointer(localX, localY);
        selectionAnchor = caret;
        selecting = true;
        blinkStart = std::chrono::steady_clock::now();
    }

    captureMouse();
    return Handled;
}

auto OMDemiurgeTextEdit::processMouseMove(float x, float y) -> OMDemiurgeEventResult
{
    if (disabled || fontset == nullptr || !selecting)
    {
        return Ignored;
    }

    float localX = x;
    float localY = y;

    try
    {
        auto bound = this->fetchLayoutBound();
        localX = x - bound.x - 8.0f + scrollX;
        localY = y - bound.y - 4.0f + scrollY;
    }
    catch (std::bad_any_cast &)
    {
    }

    {
        std::lock_guard<std::mutex> lock(stateMutex);
        caret = caretFromPointer(localX, localY);
        blinkStart = std::chrono::steady_clock::now();
    }

    return Handled;
}

auto OMDemiurgeTextEdit::processMouseUp(float x, float y, uint8_t button) -> OMDemiurgeEventResult
{
    selecting = false;
    return Handled;
}

auto OMDemiurgeTextEdit::processKeyDown(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    bool changed = false;
    bool release = false;
    OMDemiurgeEventResult result = Handled;
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
            else
            {
                result = Ignored;
            }

            if (event.keycode == SDLK_RETURN || event.keycode == SDLK_KP_ENTER)
            {
                result = Handled;
            }
        }
        else
        {
            switch (event.keycode)
            {
            case SDLK_BACKSPACE:
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
            case SDLK_DELETE:
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
            case SDLK_UP:
                if (shift && !hasSelection())
                {
                    selectionAnchor = caret;
                }

                moveVertical(-1);

                if (!shift)
                {
                    selectionAnchor = caret;
                }
                break;
            case SDLK_DOWN:
                if (shift && !hasSelection())
                {
                    selectionAnchor = caret;
                }

                moveVertical(1);

                if (!shift)
                {
                    selectionAnchor = caret;
                }
                break;
            case SDLK_HOME:
                if (shift && !hasSelection())
                {
                    selectionAnchor = caret;
                }

                caret = lineStartOf(caret);

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

                caret = lineEndOf(caret);

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
            case SDLK_KP_ENTER: {
                if (deleteSelection() || codepointCount(text) < maxLength)
                {
                    text.insert(caret, "\n");
                    ++caret;
                    selectionAnchor = caret;
                    blinkStart = std::chrono::steady_clock::now();
                    changed = true;
                }
                break;
            }
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

auto OMDemiurgeTextEdit::processTextInput(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
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

auto OMDemiurgeTextEdit::processTextEditing(const OMDemiurgeEvent &event) -> OMDemiurgeEventResult
{
    if (disabled)
    {
        return Ignored;
    }

    bool changed = false;

    {
        std::lock_guard<std::mutex> lock(stateMutex);

        if (event.text.empty())
        {
            composingEnded = true;
            composingEndedAt = std::chrono::steady_clock::now();
            return Handled;
        }

        changed = deleteSelection();
        composingEnded = false;
        composing = event.text;
        lastComposingAt = std::chrono::steady_clock::now();
        blinkStart = std::chrono::steady_clock::now();
    }

    if (changed)
    {
        notifyChange();
    }

    return Handled;
}

auto OMDemiurgeTextEdit::processFocusGained() -> void
{
    common::OMTextInputController::instance().setTextFocus(true);
    common::OMTextInputController::instance().setActive(true);
    focusAmount.animateTo(1.0f, easeOutCirc<float>, 0.15f);
    blinkStart = std::chrono::steady_clock::now();
}

auto OMDemiurgeTextEdit::processFocusLost() -> void
{
    common::OMTextInputController::instance().setTextFocus(false);
    common::OMTextInputController::instance().setActive(false);

    std::lock_guard<std::mutex> lock(stateMutex);
    composing.clear();
    composingEnded = false;
    selectionAnchor = caret;
    selecting = false;
    focusAmount.animateTo(0.0f, easeOutCirc<float>, 0.15f);
}

void OMDemiurgeTextEdit::setText(const std::string &value)
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

auto OMDemiurgeTextEdit::getText() const -> std::string
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return text;
}

void OMDemiurgeTextEdit::setPlaceholder(const std::string &value)
{
    std::lock_guard<std::mutex> lock(stateMutex);
    placeholder = value;
}

auto OMDemiurgeTextEdit::getPlaceholder() const -> std::string
{
    std::lock_guard<std::mutex> lock(stateMutex);
    return placeholder;
}

void OMDemiurgeTextEdit::setPlaceholderColor(const glm::vec4 &color)
{
    placeholderColor.animateTo(color, easeOutCirc<float>, stylesStorage.get<float>("animationSpeed", 0.18f));
}

void OMDemiurgeTextEdit::setMaxLength(int codepoints)
{
    maxLength = std::max(1, codepoints);
}

void OMDemiurgeTextEdit::setOnChange(std::function<void(const std::string &)> handler)
{
    handleChange = std::move(handler);
}

void OMDemiurgeTextEdit::setTextHeight(int height)
{
    stylesStorage.put("textHeight", height);
}

void OMDemiurgeTextEdit::setColors(glm::vec4 background, glm::vec4 text, glm::vec4 caret)
{
    backgroundColor.animateTo(background, easeOutCirc<float>, 0.18f);
    textColor.animateTo(text, easeOutCirc<float>, 0.18f);
    caretColor.animateTo(caret, easeOutCirc<float>, 0.18f);
}

void OMDemiurgeTextEdit::setDisabled(bool value, bool animate)
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
