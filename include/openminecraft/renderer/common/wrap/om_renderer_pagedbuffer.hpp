#ifndef OM_RENDERER_PAGEDBUFFER_HPP
#define OM_RENDERER_PAGEDBUFFER_HPP

#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <functional>
#include <vector>
namespace openminecraft::renderer::common::wrap
{
class OMPagedBuffer
{
  public:
    OMPagedBuffer(OMRenderer *renderer, uint32_t pageSize);
    ~OMPagedBuffer();

    auto expand() -> void;
    auto allocatePage() -> uint32_t;
    auto freePage(uint32_t pageIndex) -> void;

    std::function<void(OMPagedBuffer *, uint32_t)> onPageFreed = [](OMPagedBuffer *, uint32_t) {};

    common::OMRendererBuffer *buffer;

  private:
    std::vector<bool> occupied;

    OMRenderer *renderer;
    uint32_t pageSize;
};
} // namespace openminecraft::renderer::common::wrap

#endif // OM_RENDERER_PAGEDBUFFER_HPP