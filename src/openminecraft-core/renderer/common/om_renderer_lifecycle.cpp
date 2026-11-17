#include "openminecraft/renderer/common/om_renderer_lifecycle.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include <iostream>

namespace openminecraft::renderer::common
{
OMRendererLifeCycle::OMRendererLifeCycle() : logger("OMRendererLifeCycle", this)
{
}

void OMRendererLifeCycle::retireBuffer(OMRendererBuffer *buffer)
{
    if (buffer == nullptr)
    {
        return;
    }

    retired.push_back({buffer, nullptr, currentTag});
}

void OMRendererLifeCycle::retireTexture(OMRendererTexture *texture)
{
    if (texture == nullptr)
    {
        return;
    }

    retired.push_back({nullptr, texture, currentTag});
}

void OMRendererLifeCycle::collect(const FrameComplete &frameComplete)
{
    uint32_t bufCount = 0, texCount = 0;
    for (auto it = retired.begin(); it != retired.end();)
    {
        const bool frameDone = frameComplete(it->tag.frameIndex, it->tag.frameSerial);

        if (!frameDone)
        {
            ++it;
            continue;
        }

        if (it->texture != nullptr && !it->texture->uploadComplete())
        {
            ++it;
            continue;
        }

        if (it->buffer != nullptr)
        {
            delete it->buffer;
            ++bufCount;
        }
        if (it->texture != nullptr)
        {
            delete it->texture;
            ++texCount;
        }

        it = retired.erase(it);
    }

    if (bufCount || texCount)
    {
        logger.debug("retired {} buffers, {} textures", bufCount, texCount);
    }
}

void OMRendererLifeCycle::clear()
{
    for (auto &entry : retired)
    {
        delete entry.buffer;
        delete entry.texture;
    }
    retired.clear();
}

OMRendererLifeCycle::~OMRendererLifeCycle()
{
    clear();
}
} // namespace openminecraft::renderer::common
