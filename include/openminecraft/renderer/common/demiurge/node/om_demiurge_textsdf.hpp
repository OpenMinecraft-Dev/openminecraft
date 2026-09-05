#ifndef OM_DEMIURGE_TEXTSDF_HPP
#define OM_DEMIURGE_TEXTSDF_HPP

#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
namespace openminecraft::renderer::common::demiurge::node
{
class OMDemiurgeTextSdfNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeTextSdfNode(geom::OMFontSet *fontset);
    virtual ~OMDemiurgeTextSdfNode();

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto remove() -> void override;
    auto syncLayout() -> void override;

  private:
    std::vector<int> glyphIds = {};
    OMDemiurgeRendererHandler *handler = nullptr;
    geom::OMFontSet *set = nullptr;

    std::vector<geom::OMFontSetShapeResult> shapeResult;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
