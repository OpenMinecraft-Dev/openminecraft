#ifndef OM_RENDERER_PIPELINE_HPP
#define OM_RENDERER_PIPELINE_HPP

#include <memory>
namespace openminecraft::renderer
{
class OMRenderer;
}

namespace openminecraft::renderer::common
{
class OMShader;
class OMRendererPipeline
{
  public:
    OMRendererPipeline(OMRenderer *renderer);

    void attachShader(std::shared_ptr<OMShader> shader);
};
} // namespace openminecraft::renderer::common

#endif
