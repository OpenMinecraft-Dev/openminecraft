#ifndef OM_RENDERER_SEGBUF_HPP
#define OM_RENDERER_SEGBUF_HPP

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <vector>
#include <algorithm>

namespace openminecraft::renderer::common::wrap
{
struct OMRendererSegBufBlock
{
    uint32_t offset;
    uint32_t length;

    auto slice(uint32_t n) -> OMRendererSegBufBlock
    {
        if (n <= length)
        {
            OMRendererSegBufBlock b = {offset, n};
            offset += n;
            length -= n;
            return b;
        }
        else
        {
            OMRendererSegBufBlock b = {offset, length};
            offset += length;
            length = 0;
            return b;
        }
    }
};

class OMRendererSegBuf
{
  public:
    OMRendererSegBuf(OMRenderer *renderer, uint32_t initialSize) : totalSize(initialSize), renderer(renderer)
    {
        buffer = renderer->allocateBuffer(InstanceData, totalSize);
        freeBlocks = {{0, totalSize}};
    }

    ~OMRendererSegBuf()
    {
        delete buffer;
    }

    auto allocate(uint32_t minSize, uint32_t maxSize, uint32_t alignment) -> OMRendererSegBufBlock
    {
        if (maxSize < minSize)
            maxSize = minSize;

        for (auto it = freeBlocks.begin(); it != freeBlocks.end(); ++it)
        {
            if (it->length >= minSize)
            {
                auto allo = std::min(it->length, maxSize);
                allo -= (allo % alignment);
                OMRendererSegBufBlock allocated = it->slice(allo);

                if (it->length == 0)
                {
                    freeBlocks.erase(it);
                }
                return allocated;
            }
        }

        expand();

        return allocate(minSize, maxSize, alignment);
    }

    void deallocate(OMRendererSegBufBlock block)
    {
        if (block.length == 0)
            return;

        auto temp = std::calloc(1, block.length);
        buffer->updateDataPart(temp, block.offset, block.length);
        std::free(temp);

        auto it = freeBlocks.begin();
        while (it != freeBlocks.end() && it->offset < block.offset)
        {
            ++it;
        }
        freeBlocks.insert(it, block);

        std::sort(freeBlocks.begin(), freeBlocks.end(),
                  [](const OMRendererSegBufBlock &a, const OMRendererSegBufBlock &b) { return a.offset < b.offset; });

        std::vector<OMRendererSegBufBlock> merged;
        for (const auto &cur : freeBlocks)
        {
            if (merged.empty() || merged.back().offset + merged.back().length != cur.offset)
            {
                merged.push_back(cur);
            }
            else
            {
                merged.back().length += cur.length;
            }
        }
        freeBlocks = std::move(merged);
    }

    void update(OMRendererSegBufBlock block, const void *data)
    {
        buffer->updateDataPart(const_cast<void *>(data), block.offset, block.length);
    }

    OMRendererBuffer *buffer;
    uint32_t totalSize;

  private:
    OMRenderer *renderer;
    std::vector<OMRendererSegBufBlock> freeBlocks;

    void expand()
    {
        uint32_t newTotal = totalSize * 2;
        OMRendererBuffer *newBuffer = renderer->allocateBuffer(InstanceData, newTotal);
        buffer->copyTo(newBuffer);
        delete buffer;
        buffer = newBuffer;

        OMRendererSegBufBlock newBlock = {totalSize, totalSize};
        freeBlocks.push_back(newBlock);

        totalSize = newTotal;

        std::sort(freeBlocks.begin(), freeBlocks.end(),
                  [](const OMRendererSegBufBlock &a, const OMRendererSegBufBlock &b) { return a.offset < b.offset; });
        std::vector<OMRendererSegBufBlock> merged;
        for (const auto &cur : freeBlocks)
        {
            if (merged.empty() || merged.back().offset + merged.back().length != cur.offset)
            {
                merged.push_back(cur);
            }
            else
            {
                merged.back().length += cur.length;
            }
        }
        freeBlocks = std::move(merged);
    }
};

} // namespace openminecraft::renderer::common::wrap

#endif