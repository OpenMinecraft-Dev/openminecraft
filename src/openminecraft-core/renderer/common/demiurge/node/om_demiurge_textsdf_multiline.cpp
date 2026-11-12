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
    if (stylesStorage.isModified())
    {
        const auto text = stylesStorage.get<std::string>("text", "");

        lines.clear();

        size_t start = 0;

        while (true)
        {
            const size_t pos = text.find('\n', start);
            lines.push_back(set->shape(text.substr(start, pos == std::string::npos ? std::string::npos : pos - start)));

            if (pos == std::string::npos)
            {
                break;
            }

            start = pos + 1;
        }

        const auto th = static_cast<float>(stylesStorage.get<int>("textHeight", 12));

        float maxWidth = 0.0f;
        float maxUnitHeight = 0.0f;

        for (const auto &line : lines)
        {
            const auto b = set->bound(line);
            maxWidth = std::max(maxWidth, b.x);
            maxUnitHeight = std::max(maxUnitHeight, b.y);
        }

        textUnitHeight = maxUnitHeight;
        lineAdvance = stylesStorage.get<float>("textLineHeight", maxUnitHeight * th * 1.25f);

        const float width = maxWidth * th;
        const float height = textUnitHeight * th + std::max<int>(0, static_cast<int>(lines.size()) - 1) * lineAdvance;

        stylesStorage.put("minWidth", width);
        stylesStorage.put("minHeight", height);
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
