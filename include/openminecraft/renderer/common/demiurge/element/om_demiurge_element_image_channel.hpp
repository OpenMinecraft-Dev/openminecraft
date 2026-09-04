#ifndef OM_DEMIURGE_ELEMENT_IMAGE_CHANNEL_HPP
#define OM_DEMIURGE_ELEMENT_IMAGE_CHANNEL_HPP

#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_image.hpp"
#include "openminecraft/renderer/common/demiurge/element/om_demiurge_element_quad_channel.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_pipeline.hpp"
#include "openminecraft/renderer/common/om_renderer_rendertarget.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <functional>
#include <unordered_map>
namespace openminecraft::renderer::common::demiurge::element
{
class OMDemiurgeImageChannel : public OMDemiurgeQuadChannel<OMDemiurgeElementImage>
{
  public:
    OMDemiurgeImageChannel(OMRenderer *renderer, std::function<void()> r) : OMDemiurgeQuadChannel(renderer, r)
    {
    }
    ~OMDemiurgeImageChannel() = default;

    auto init(OMRendererBuffer *uniform, OMRendererRenderTarget *target) -> void override;
    auto submitTask(OMRendererTask *task, float upper, float lower) -> void override;
    auto destroy() -> void override
    {
        delete instanceBuffer;
        for (auto p : pipelines)
        {
            delete p.second;
        }
    }

    auto onRemove(int i) -> void override
    {
        textures.erase(i);
        recreation();
    }

    OMRendererBuffer *uniform;
    OMRendererRenderTarget *target;
    std::unordered_map<int, OMRendererTexture *> textures;
    std::unordered_map<int, OMRendererPipeline *> pipelines;
};
} // namespace openminecraft::renderer::common::demiurge::element

#endif
