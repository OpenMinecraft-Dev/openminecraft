#ifndef OM_RENDERER_OBJECT_REF_HPP
#define OM_RENDERER_OBJECT_REF_HPP

#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <atomic>
#include <cstdint>
#include <string>
#include <utility>

namespace openminecraft::renderer::common::wrap
{
// Globally unique object tag counter: a static in a template class is per-instantiation, so it lives in a non-template function
inline auto nextObjectTagId() -> uint64_t
{
    static std::atomic<uint64_t> counter = 0;
    return ++counter;
}

// Lightweight reference: stores only the registry string tag; the object itself is owned/destroyed by OMRenderer.
class OMBufferRef
{
  public:
    OMBufferRef() = default;
    OMBufferRef(OMRenderer *renderer, std::string tag) : renderer(renderer), tag(std::move(tag))
    {
    }

    auto operator->() const -> common::OMRendererBuffer *
    {
        return renderer->getBuffer(tag);
    }

    auto get() const -> common::OMRendererBuffer *
    {
        return renderer->getBuffer(tag);
    }

    // Compatibility for legacy call sites: still resolved through the registry after implicit conversion
    operator common::OMRendererBuffer *() const
    {
        return get();
    }

    auto getTag() const -> const std::string &
    {
        return tag;
    }

    explicit operator bool() const
    {
        return renderer != nullptr && !tag.empty();
    }

  private:
    OMRenderer *renderer = nullptr;
    std::string tag;
};

class OMTextureRef
{
  public:
    OMTextureRef() = default;
    OMTextureRef(OMRenderer *renderer, std::string tag) : renderer(renderer), tag(std::move(tag))
    {
    }

    auto operator->() const -> common::OMRendererTexture *
    {
        return renderer->getTexture(tag);
    }

    auto get() const -> common::OMRendererTexture *
    {
        return renderer->getTexture(tag);
    }

    // Compatibility for legacy call sites: still resolved through the registry after implicit conversion
    operator common::OMRendererTexture *() const
    {
        return get();
    }

    auto getTag() const -> const std::string &
    {
        return tag;
    }

    explicit operator bool() const
    {
        return renderer != nullptr && !tag.empty();
    }

  private:
    OMRenderer *renderer = nullptr;
    std::string tag;
};

class OMRenderTargetRef
{
  public:
    OMRenderTargetRef() = default;
    OMRenderTargetRef(OMRenderer *renderer, std::string tag) : renderer(renderer), tag(std::move(tag))
    {
    }

    auto operator->() const -> common::OMRendererRenderTarget *
    {
        return renderer->getRenderTarget(tag);
    }

    auto get() const -> common::OMRendererRenderTarget *
    {
        return renderer->getRenderTarget(tag);
    }

    // Compatibility for legacy call sites: still resolved through the registry after implicit conversion
    operator common::OMRendererRenderTarget *() const
    {
        return get();
    }

    auto getTag() const -> const std::string &
    {
        return tag;
    }

    explicit operator bool() const
    {
        return renderer != nullptr && !tag.empty();
    }

  private:
    OMRenderer *renderer = nullptr;
    std::string tag;
};
} // namespace openminecraft::renderer::common::wrap

#endif
