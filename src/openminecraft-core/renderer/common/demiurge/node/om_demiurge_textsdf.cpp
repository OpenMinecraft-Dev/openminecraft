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
        auto th = stylesStorage.get<int>("textheight", 12);

        stylesStorage.put("minWidth", b.x * th);
        stylesStorage.put("minHeight", b.y * th);
    }
    OMDemiurgeNode::syncLayout();
}

auto OMDemiurgeTextSdfNode::submit(OMDemiurgeRendererHandler *handler, float depth) -> void
{
    this->handler = handler;
    if (stylesStorage.isModified())
    {
        auto pp = stylesStorage.get<OMDemiurgeRect>("layoutBound");

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

        auto th = stylesStorage.get<int>("textheight", 12);

        for (int i = 0; i < s.size(); ++i)
        {
            auto t = ch->temporary(glyphIds[i]);
            t->color = genLinear(stylesStorage.get<int>("color", 0));
            t->position = {
                pp.x + s[i].position.x * th,
                pp.y + s[i].position.y * th,
                s[i].size.x * th,
                s[i].size.y * th,
            };
            t->depth = depth;
            t->factor = stylesStorage.get<float>(
                "factor", 0.05f - 0.0005f * (glm::clamp(static_cast<float>(th), 12.0f, 92.0f) - 12.0f));
            t->glyphIndex = ch->storeGlyph(s[i]);
        }

        stylesStorage.solve();
    }

    for (auto c : children)
    {
        c->submit(handler, depth - layerHalfWidth * 2);
    }
}

auto OMDemiurgeTextSdfNode::remove() -> void
{
    for (auto r : glyphIds)
    {
        handler->fetchFontChannel(set)->remove(r);
    }
    handler = nullptr;

    for (auto c : children)
    {
        c->remove();
    }
}
} // namespace openminecraft::renderer::common::demiurge::node
