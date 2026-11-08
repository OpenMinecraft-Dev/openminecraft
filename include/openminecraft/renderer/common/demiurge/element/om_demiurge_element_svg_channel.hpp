#ifndef OM_DEMIURGE_ELEMENT_SVG_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_SVG_CHANNEL_HPP

#include "glm/ext/vector_float2.hpp"
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/geom/om_fontset.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_svg.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include <string>
#include <unordered_map>
#include <vector>
namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeSvgChannel : public OMDemiurgeQuadChannel<OMDemiurgeElementSvg>
{
  public:
    OMDemiurgeSvgChannel(OMRenderer *renderer, std::function<void()> f) : OMDemiurgeQuadChannel(renderer, f)
    {
        static std::atomic<uint64_t> counter = 0;
        baseGlyphTag = "demiurge.svg.glyph." + std::to_string(++counter);
    }
    virtual ~OMDemiurgeSvgChannel() = default;

    void init(const std::string &uniformTag, const std::string &targetTag) override;
    void destroy() override
    {
        if (glyphBuffer)
        {
            renderer->unregisterBuffer(glyphBuffer.getTag());
            glyphBuffer = wrap::OMBufferRef();
        }
        OMDemiurgeQuadChannel::destroy();
    }

    auto storeGlyph(std::string, glm::vec2 size) -> int;

  private:
    wrap::OMBufferRef glyphBuffer;
    std::string glyphTag;
    std::string baseGlyphTag;
    uint64_t glyphGeneration = 0;

    std::unordered_map<binary::hash::hash_t, uint32_t> glyphOffsets = {};
    std::vector<float> glyphData = {};
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
