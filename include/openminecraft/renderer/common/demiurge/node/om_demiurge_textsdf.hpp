#ifndef OM_DEMIURGE_TEXTSDF_HPP
#define OM_DEMIURGE_TEXTSDF_HPP

#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/om_demiurge_node.hpp"
namespace openminecraft::renderer::common::demiurge::node
{
class OMDemiurgeTextSdfNode : public OMDemiurgeNode
{
  public:
    OMDemiurgeTextSdfNode(fontproc::OMFontSet *fontset);
    virtual ~OMDemiurgeTextSdfNode();

    auto submit(OMDemiurgeRendererHandler *handler, float depth) -> void override;
    auto remove() -> void override;

  private:
    std::vector<int> glyphIds = {};
    OMDemiurgeRendererHandler *handler = nullptr;
    fontproc::OMFontSet *set = nullptr;
};
} // namespace openminecraft::renderer::common::demiurge::node

#endif
