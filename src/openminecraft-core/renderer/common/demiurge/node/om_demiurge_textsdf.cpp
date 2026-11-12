#include <utility>

#include "openminecraft/renderer/common/demiurge/node/om_demiurge_textsdf.hpp"
#include "glm/common.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_geometry.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_srgb.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_rendererhandler.hpp"

namespace openminecraft::renderer::common::demiurge::node
{
OMDemiurgeTextSdfNode::OMDemiurgeTextSdfNode(geom::OMFontSet *f) : set(std::move(f))
{
}
OMDemiurgeTextSdfNode::~OMDemiurgeTextSdfNode() = default;

auto OMDemiurgeTextSdfNode::syncLayout() -> void
{
    if (stylesStorage.isModified())
    {
        shapeResult = set->shape(stylesStorage.get<std::string>("text", ""));
        auto b = set->bound(shapeResult);
        auto th = stylesStorage.get<int>("textHeight", 12);

        stylesStorage.put("minWidth", b.x * th);
        stylesStorage.put("minHeight", b.y * th);
    }
    OMDemiurgeNode::syncLayout();
}

auto OMDemiurgeTextSdfNode::submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void
{
    this->handler = handler;
    if (stylesStorage.isModified())
    {
        auto layoutBound = stylesStorage.get<OMDemiurgeRect>("layoutBound");

        auto text = stylesStorage.get<std::string>("text", "");
        auto &s = shapeResult;
        auto ch = handler->fetchFontChannel(set);

        if (s.size() > glyphIds.size())
        {
            while (s.size() != glyphIds.size())
            {
                glyphIds.push_back(ch->request(depth));
            }
        }
        else if (s.size() < glyphIds.size())
        {
            while (s.size() != glyphIds.size())
            {
                ch->remove(glyphIds.back());
                glyphIds.pop_back();
            }
        }

        auto th = stylesStorage.get<int>("textHeight", 12);
        const float opacity = fetchOpacity();
        const glm::vec2 textScale = stylesStorage.get<glm::vec2>("scale", glm::vec2(1.0f));
        const glm::vec2 textCenter(layoutBound.x + layoutBound.width * 0.5f,
                                   layoutBound.y + layoutBound.height * 0.5f);

        for (int i = 0; i < s.size(); ++i)
        {
            auto t = ch->temporary(glyphIds[i]);
            t->color = genLinear(stylesStorage.get<int>("color", 0));
            const glm::vec2 glyphPos(layoutBound.x + s[i].position.x * th, layoutBound.y + s[i].position.y * th);
            const glm::vec2 glyphSize(s[i].size.x * th, s[i].size.y * th);
            const glm::vec2 glyphCenter = glyphPos + glyphSize * 0.5f;
            const glm::vec2 scaledCenter = textCenter + (glyphCenter - textCenter) * textScale;

            t->position = {scaledCenter.x - glyphSize.x * 0.5f, scaledCenter.y - glyphSize.y * 0.5f, glyphSize.x,
                           glyphSize.y};
            t->scale = textScale;
            t->depth = depth;
            t->factor = stylesStorage.get<float>("factor", 0.025f);
            t->glyphIndex = ch->storeGlyph(s[i]);
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

auto OMDemiurgeTextSdfNode::remove() -> void
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
