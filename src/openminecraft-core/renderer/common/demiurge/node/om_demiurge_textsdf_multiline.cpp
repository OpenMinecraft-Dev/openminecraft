#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf_multiline.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

#include <algorithm>

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeTextSdfMultilineNode::OMDemiurgeTextSdfMultilineNode(geom::OMFontSet *f) : set(f)
{
}

OMDemiurgeTextSdfMultilineNode::~OMDemiurgeTextSdfMultilineNode() = default;

auto OMDemiurgeTextSdfMultilineNode::syncLayout() -> void
{
    const bool wrap = stylesStorage.get<bool>("wrap", false);

    float boundWidth = 0.0f;
    try
    {
        boundWidth = stylesStorage.get<OMDemiurgeRect>("layoutBound").width;
    }
    catch (const std::bad_any_cast &)
    {
    }

    bool forceReflow = false;

    if (!wrapStateInitialized || wrap != wrapEnabled)
    {
        wrapStateInitialized = true;
        wrapEnabled = wrap;
        forceReflow = true;
        wrapSizingPending = wrap;

        if (wrap)
        {
            // Wrapping is driven by the current layout bound, so the old
            // content-sized minimums must not constrain Yoga any more.
            stylesStorage.remove("minWidth");
            stylesStorage.remove("minHeight");
            YGNodeStyleSetMinWidth(yogaNode, 0.0f);
            YGNodeStyleSetMinHeight(yogaNode, 0.0f);
        }

        lastWrapBoundWidth = -1.0f;
        hasShapedText = false;
    }

    if (wrap && boundWidth != lastWrapBoundWidth)
    {
        lastWrapBoundWidth = boundWidth;
        forceReflow = true;
        hasShapedText = false;
    }

    if (stylesStorage.isModified() || forceReflow)
    {
        const auto text = stylesStorage.get<std::string>("text", "");
        const auto th = stylesStorage.get<int>("textHeight", 12);

        if (forceReflow || !hasShapedText || text != lastShapedText || th != lastShapedTextHeight)
        {
            lines.clear();

            auto appendExplicitLines = [&](const std::string &value) {
                size_t start = 0;
                while (true)
                {
                    const size_t pos = value.find('\n', start);
                    lines.push_back(set->shape(value.substr(start, pos == std::string::npos ? std::string::npos : pos - start)));
                    if (pos == std::string::npos)
                    {
                        break;
                    }
                    start = pos + 1;
                }
            };

            auto appendWrappedLines = [&](const std::string &value, float limit, float textHeight) {
                if (limit <= 0.0f)
                {
                    appendExplicitLines(value);
                    return;
                }

                const float unitLimit = limit / std::max(textHeight, 1.0f);
                size_t paragraphStart = 0;

                do
                {
                    const size_t newline = value.find('\n', paragraphStart);
                    const std::string paragraph =
                        value.substr(paragraphStart, newline == std::string::npos ? std::string::npos : newline - paragraphStart);
                    paragraphStart = newline == std::string::npos ? std::string::npos : newline + 1;

                    if (paragraph.empty())
                    {
                        lines.push_back({});
                        continue;
                    }

                    std::vector<std::string> tokens;
                    std::string token;
                    bool tokenIsSpace = false;
                    for (char c : paragraph)
                    {
                        const bool isSpace = c == ' ' || c == '\t';
                        if (token.empty() || isSpace == tokenIsSpace)
                        {
                            token.push_back(c);
                            tokenIsSpace = isSpace;
                        }
                        else
                        {
                            tokens.push_back(token);
                            token = std::string(1, c);
                            tokenIsSpace = isSpace;
                        }
                    }
                    if (!token.empty())
                    {
                        tokens.push_back(token);
                    }

                    std::string current;
                    auto flushLine = [&]() {
                        lines.push_back(set->shape(current));
                        current.clear();
                    };

                    for (const auto &part : tokens)
                    {
                        const std::string candidate = current + part;
                        if (!current.empty() && set->advance(candidate) > unitLimit)
                        {
                            flushLine();
                            if (part.find_first_not_of(" \t") == std::string::npos)
                            {
                                continue;
                            }
                            current = part;
                        }
                        else
                        {
                            current = candidate;
                        }

                        if (set->advance(current) <= unitLimit)
                        {
                            continue;
                        }

                        std::string remainder;
                        for (char c : current)
                        {
                            const std::string next = remainder + std::string(1, c);
                            if (!remainder.empty() && set->advance(next) > unitLimit)
                            {
                                lines.push_back(set->shape(remainder));
                                remainder = std::string(1, c);
                            }
                            else
                            {
                                remainder = next;
                            }
                        }
                        current = remainder;
                    }

                    if (!current.empty())
                    {
                        flushLine();
                    }
                } while (paragraphStart != std::string::npos);
            };

            if (wrap)
            {
                appendWrappedLines(text, boundWidth, static_cast<float>(th));
            }
            else
            {
                appendExplicitLines(text);
            }

            lastShapedText = text;
            lastShapedTextHeight = th;
            hasShapedText = true;
        }

        const float textHeight = static_cast<float>(th);
        float contentWidth = 0.0f;
        float maxUnitHeight = 0.0f;

        for (const auto &line : lines)
        {
            const auto b = set->bound(line);
            contentWidth = std::max(contentWidth, b.x);
            maxUnitHeight = std::max(maxUnitHeight, b.y);
        }

        textUnitHeight = maxUnitHeight;
        lineAdvance = stylesStorage.get<float>("textLineHeight", maxUnitHeight * textHeight * 1.25f);

        const float width = contentWidth * textHeight;
        const float height =
            textUnitHeight * textHeight + std::max<int>(0, static_cast<int>(lines.size()) - 1) * lineAdvance;

        if (wrap)
        {
            if (wrapSizingPending)
            {
                // The bound used above can still be the previous content-sized
                // bound. Leave both minimums cancelled for one Yoga pass so the
                // real parent/width-constrained bound is computed; the next
                // syncLayout wraps against that bound.
                YGNodeStyleSetMinWidth(yogaNode, 0.0f);
                YGNodeStyleSetMinHeight(yogaNode, 0.0f);
                wrapSizingPending = false;
            }
            else
            {
                // Re-apply the dimensions produced by the bound-width layout so
                // the wrapped box participates in Yoga sizing and scroll height.
                const float wrappedWidth = boundWidth > 0.0f ? boundWidth : width;
                stylesStorage.putLayout("minWidth", wrappedWidth);
                stylesStorage.putLayout("minHeight", height);
                YGNodeStyleSetMinWidth(yogaNode, wrappedWidth);
                YGNodeStyleSetMinHeight(yogaNode, height);
            }
        }
        else
        {
            stylesStorage.putLayout("minWidth", width);
            stylesStorage.putLayout("minHeight", height);
            YGNodeStyleSetMinWidth(yogaNode, width);
            YGNodeStyleSetMinHeight(yogaNode, height);
        }
    }

    OMDemiurgeNode::syncLayout();
}

auto OMDemiurgeTextSdfMultilineNode::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    this->handler = handler;

    if (stylesStorage.isModified())
    {
        const auto layoutBound = stylesStorage.get<OMDemiurgeRect>("layoutBound");
        const auto ch = handler->fetchFontChannel(set);
        const auto th = static_cast<float>(stylesStorage.get<int>("textHeight", 12));

        size_t total = 0;

        for (const auto &line : lines)
        {
            total += line.size();
        }

        if (total > glyphIds.size())
        {
            while (total != glyphIds.size())
            {
                glyphIds.push_back(ch->request(depth));
            }
        }
        else if (total < glyphIds.size())
        {
            while (total != glyphIds.size())
            {
                ch->remove(glyphIds.back());
                glyphIds.pop_back();
            }
        }

        size_t index = 0;
        const float opacity = fetchOpacity();
        const glm::vec2 textScale = stylesStorage.get<glm::vec2>("scale", glm::vec2(1.0f));
        const glm::vec2 textCenter(layoutBound.x + layoutBound.width * 0.5f,
                                   layoutBound.y + layoutBound.height * 0.5f);

        for (size_t li = 0; li < lines.size(); ++li)
        {
            const float lineY = layoutBound.y + static_cast<float>(li) * lineAdvance;

            for (const auto &glyph : lines[li])
            {
                auto t = ch->temporary(glyphIds[index++]);
                t->color = genLinear(stylesStorage.get<int>("color", 0));
                t->color.a *= opacity;
                const glm::vec2 glyphPos(layoutBound.x + glyph.position.x * th, lineY + glyph.position.y * th);
                const glm::vec2 glyphSize(glyph.size.x * th, glyph.size.y * th);
                const glm::vec2 glyphCenter = glyphPos + glyphSize * 0.5f;
                const glm::vec2 scaledCenter = textCenter + (glyphCenter - textCenter) * textScale;

                t->position = {scaledCenter.x - glyphSize.x * 0.5f, scaledCenter.y - glyphSize.y * 0.5f, glyphSize.x,
                               glyphSize.y};
                t->scale = textScale;
                t->depth = depth;
                t->factor = stylesStorage.get<float>("factor", 0.025f);
                t->glyphIndex = ch->storeGlyph(glyph);
            }
        }

        stylesStorage.solve();
    }

    if (!glyphIds.empty())
    {
        auto clip = resolveClipBound();
        auto ch = handler->fetchFontChannel(set);

        for (auto id : glyphIds)
        {
            handler->applyClip(*ch, id, clip);
        }
    }

    for (auto c : children)
    {
        c->submit(handler, c->isLayerOverlay() ? overlayLayer(depth + 1) : depth + 1);
    }
}

auto OMDemiurgeTextSdfMultilineNode::remove() -> void
{
    if (handler != nullptr)
    {
        for (auto r : glyphIds)
        {
            handler->fetchFontChannel(set)->remove(r);
        }
    }

    // Elements were removed from the channel; drop their ids so a later
    // invalidate()+submit requests fresh ones instead of reusing dead slots.
    glyphIds.clear();
    handler = nullptr;
    stylesStorage.invalidate();

    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node
