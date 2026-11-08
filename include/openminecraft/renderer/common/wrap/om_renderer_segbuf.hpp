#ifndef OM_RENDERER_SEGBUF_HPP
#define OM_RENDERER_SEGBUF_HPP

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <iostream>
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
    OMRendererSegBuf(OMRenderer *renderer, std::string tag, uint32_t initialSize)
        : renderer(renderer), bufferTag(std::move(tag)), totalSize(initialSize)
    {
        renderer->registerBuffer(bufferTag, InstanceData, totalSize);
        freeBlocks = {{0, totalSize}};

        shadow.resize(totalSize, 0);

        buffer()->updateData(shadow.data());
    }

    ~OMRendererSegBuf()
    {
        for (auto &r : retiredBuffers)
        {
            renderer->unregisterBuffer(r.first);
        }

        retiredBuffers.clear();
        renderer->unregisterBuffer(bufferTag);
    }

    auto buffer() -> OMRendererBuffer *
    {
        return renderer->getBuffer(bufferTag);
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

        // freeBlocks stays sorted on insert, so only a linear merge of adjacent blocks is needed
        mergeFreeBlocks();
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

    // Default upload budget per flush: avoid multi-hundred-MB uploads causing tens of ms of stutter.
    // Unfinished ranges resume next frame (at most 6 layers x this value per frame).
    static constexpr uint64_t flushBudgetBytes = 1 * 1024 * 1024;

    void flushDirty(uint64_t budget = flushBudgetBytes)
    {
        ++frameCounter;

        for (size_t i = 0; i < retiredBuffers.size();)
        {
            if (frameCounter - retiredBuffers[i].second > 8)
            {
                renderer->unregisterBuffer(retiredBuffers[i].first);
                retiredBuffers.erase(retiredBuffers.begin() + static_cast<std::ptrdiff_t>(i));
            }
            else
            {
                ++i;
            }
        }

        uint64_t spent = 0;
        size_t consumed = 0;

        while (consumed < dirtyRanges.size() && spent < budget)
        {
            auto &range = dirtyRanges[consumed];
            const uint64_t length = static_cast<uint64_t>(range.end - range.begin);
            const uint64_t available = budget - spent;
            const uint32_t part = static_cast<uint32_t>(std::min<uint64_t>(length, available));

            buffer()->updateDataPart(shadow.data() + range.begin, range.begin, part);
            spent += part;

            if (part < length)
            {
                range.begin += part;
                break;
            }

            ++consumed;
        }

        dirtyRanges.erase(dirtyRanges.begin(), dirtyRanges.begin() + static_cast<std::ptrdiff_t>(consumed));
    }

    void update(OMRendererSegBufBlock block, const void *data)
    {
        std::memcpy(shadow.data() + block.offset, data, block.length);
        markDirty(block.offset, block.length);
    }

    std::string bufferTag;
    uint32_t generation = 0;
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

    struct OMRendererSegBufDirtyRange
    {
        uint32_t begin;
        uint32_t end;
    };

    // Adjacent/overlapping dirty ranges merge when the gap is <= 16KB, avoiding too many per-block upload calls without treating
    // the whole buffer as dirty (the old [dirtyMin, dirtyMax] scheme degraded to full uploads when chunks were scattered).
    static constexpr uint32_t dirtyMergeGap = 16 * 1024;
    std::vector<OMRendererSegBufDirtyRange> dirtyRanges;
    uint32_t frameCounter = 0;
    std::vector<std::pair<std::string, uint32_t>> retiredBuffers;

    void markDirty(uint32_t offset, uint32_t length)
    {
        if (length == 0)
        {
            return;
        }

        const uint32_t end = offset + length;

        size_t pos = 0;
        while (pos < dirtyRanges.size() && dirtyRanges[pos].begin < offset)
        {
            ++pos;
        }

        dirtyRanges.insert(dirtyRanges.begin() + std::ptrdiff_t(pos), OMRendererSegBufDirtyRange{offset, end});

        std::vector<OMRendererSegBufDirtyRange> merged;
        merged.reserve(dirtyRanges.size());

        for (const auto &range : dirtyRanges)
        {
            if (merged.empty() || static_cast<uint64_t>(merged.back().end) + dirtyMergeGap < range.begin)
            {
                merged.push_back(range);
            }
            else
            {
                merged.back().end = std::max(merged.back().end, range.end);
            }
        }

        dirtyRanges.swap(merged);
    }

    void normalizeFreeBlocks()
    {
        std::sort(freeBlocks.begin(), freeBlocks.end(),
                  [](const OMRendererSegBufBlock &a, const OMRendererSegBufBlock &b) { return a.offset < b.offset; });
        mergeFreeBlocks();
    }

    // Linearly merge adjacent free blocks (assumes sorted by offset)
    void mergeFreeBlocks()
    {
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
        const std::string newTag = bufferTag + ".g" + std::to_string(++generation);
        auto *newBuffer = renderer->registerBuffer(newTag, InstanceData, newTotal);
        buffer()->copyTo(newBuffer);

        // Zero the new region after the old data
        shadow.resize(newTotal, 0);
        newBuffer->updateDataPart(shadow.data() + totalSize, totalSize, totalSize);

        // The old GPU buffer may still be referenced by in-flight frames: release it a few frames later
        retiredBuffers.emplace_back(bufferTag, frameCounter);
        bufferTag = newTag;

        freeBlocks.push_back({totalSize, totalSize});

        totalSize = newTotal;

        normalizeFreeBlocks();
    }
};

} // namespace openminecraft::renderer::common::wrap

#endif
