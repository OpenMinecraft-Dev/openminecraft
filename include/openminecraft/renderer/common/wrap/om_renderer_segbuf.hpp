#ifndef OM_RENDERER_SEGBUF_HPP
#define OM_RENDERER_SEGBUF_HPP

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <list>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstring>

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

        shadow.resize(totalSize, 0);

        buffer->updateData(shadow.data());
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
                auto allo = it->length < maxSize ? it->length : maxSize;
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

        std::memset(shadow.data() + block.offset, 0, block.length);
        markDirty(block.offset, block.length);

        auto it = freeBlocks.begin();
        while (it != freeBlocks.end() && it->offset < block.offset)
        {
            ++it;
        }
        freeBlocks.insert(it, block);

        normalizeFreeBlocks();
    }

    auto compact(std::list<OMRendererSegBufBlock> blocks) -> std::list<OMRendererSegBufBlock>
    {
        blocks.sort([](const OMRendererSegBufBlock &a, const OMRendererSegBufBlock &b) { return a.offset < b.offset; });

        std::list<OMRendererSegBufBlock> result;

        for (auto &block : blocks)
        {
            if (block.length == 0)
            {
                continue;
            }

            auto it = freeBlocks.begin();

            while (it != freeBlocks.end() && (it->length < block.length || it->offset >= block.offset))
            {
                ++it;
            }

            if (it == freeBlocks.end())
            {
                result.push_back(block);
                continue;
            }

            auto target = it->slice(block.length);

            if (it->length == 0)
            {
                freeBlocks.erase(it);
            }

            std::memmove(shadow.data() + target.offset, shadow.data() + block.offset, block.length);
            std::memset(shadow.data() + block.offset, 0, block.length);

            markDirty(target.offset, block.length);
            markDirty(block.offset, block.length);

            freeBlocks.push_back(block);

            block.offset = target.offset;
            result.push_back(block);
        }

        normalizeFreeBlocks();

        return result;
    }

    void flushDirty()
    {
        if (dirtyMax <= dirtyMin)
        {
            return;
        }

        buffer->updateDataPart(shadow.data() + dirtyMin, dirtyMin, dirtyMax - dirtyMin);
        dirtyMin = UINT32_MAX;
        dirtyMax = 0;
    }

    void update(OMRendererSegBufBlock block, const void *data)
    {
        std::memcpy(shadow.data() + block.offset, data, block.length);
        markDirty(block.offset, block.length);
    }

    OMRendererBuffer *buffer;
    uint32_t totalSize;

    auto usedSize() -> uint32_t
    {
        if (freeBlocks.empty())
        {
            return totalSize;
        }

        return freeBlocks.back().offset;
    }

    auto blockCount() -> uint32_t
    {
        return freeBlocks.size();
    }

  private:
    OMRenderer *renderer;
    std::vector<OMRendererSegBufBlock> freeBlocks;
    std::vector<uint8_t> shadow;
    uint32_t dirtyMin = UINT32_MAX;
    uint32_t dirtyMax = 0;

    void markDirty(uint32_t offset, uint32_t length)
    {
        dirtyMin = std::min(dirtyMin, offset);
        dirtyMax = std::max(dirtyMax, offset + length);
    }

    void normalizeFreeBlocks()
    {
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

    void expand()
    {
        uint32_t newTotal = totalSize * 2;
        OMRendererBuffer *newBuffer = renderer->allocateBuffer(InstanceData, newTotal);
        buffer->copyTo(newBuffer);

        // 旧数据之后的新区域清零
        shadow.resize(newTotal, 0);
        newBuffer->updateDataPart(shadow.data() + totalSize, totalSize, totalSize);

        delete buffer;
        buffer = newBuffer;

        freeBlocks.push_back({totalSize, totalSize});

        totalSize = newTotal;

        normalizeFreeBlocks();
    }
};

} // namespace openminecraft::renderer::common::wrap

#endif
