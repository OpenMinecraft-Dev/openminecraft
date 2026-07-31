#ifndef OM_DEMIURGE_ELEMENT_TEXTSDF_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_TEXTSDF_CHANNEL_HPP

#include "openminecraft/fontproc/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_textsdf.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include <unordered_map>
#include <vector>
namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeTextSdfChannel : public OMDemiurgeQuadChannel<OMDemiurgeElementTextSdf>
{
  public:
    OMDemiurgeTextSdfChannel(OMRenderer *renderer, std::function<void()> f, fontproc::OMFontSet *set)
        : OMDemiurgeQuadChannel(renderer, f), fontSet(set)
    {
    }
    ~OMDemiurgeTextSdfChannel() = default;

    void init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) override;
    void destroy() override
    {
        delete glyphBuffer;

        OMDemiurgeQuadChannel::destroy();
    }

    auto storeGlyph(fontproc::OMFontSetShapeResult) -> int;

  private:
    fontproc::OMFontSet *fontSet;
    OMRendererBuffer *glyphBuffer;

    std::unordered_map<uint64_t, uint32_t> glyphOffsets = {};
    std::vector<float> glyphData = {};
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
