#ifndef OM_DEMIURGE_TEXTSDF_MULTILINE_HPP
#define OM_DEMIURGE_TEXTSDF_MULTILINE_HPP

#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
#include <vector>

namespace openminecraft::renderer::common::demiurge::node
{
// Multi-line SDF text: same rendering as OMDemiurgeTextSdfNode, but '\n' starts
// a new line. The layout height accounts for every line.
class OMDemiurgeTextSdfMultilineNode : public OMDemiurgeNode
{
  public:
    explicit OMDemiurgeTextSdfMultilineNode(geom::OMFontSet *fontset);
    virtual ~OMDemiurgeTextSdfMultilineNode();

    auto submit(OMDemiurgeRendererHandler *handler, uint32_t depth) -> void override;
    auto remove() -> void override;
    auto syncLayout() -> void override;

  private:
    std::vector<std::vector<geom::OMFontSetShapeResult>> lines;
    std::vector<int> glyphIds = {};
    OMDemiurgeRendererHandler *handler = nullptr;
    geom::OMFontSet *set = nullptr;

    float textUnitHeight = 0.0f;
    float lineAdvance = 0.0f;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
