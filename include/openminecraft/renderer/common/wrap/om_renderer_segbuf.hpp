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

    // 单次 flush 的默认上传预算：避免一次性上传几百 MB 造成几十 ms 的卡顿。
    // 未传完的区间会留到下一帧继续（每帧最多 6 个图层 × 该值）。
    static constexpr uint64_t flushBudgetBytes = 1 * 1024 * 1024;

    void flushDirty(uint64_t budget = flushBudgetBytes, uint64_t *flushedBytes = nullptr)
    {
        ++frameCounter;

        for (size_t i = 0; i < retiredBuffers.size();)
        {
            if (frameCounter - retiredBuffers[i].second > 8)
            {
                delete retiredBuffers[i].first;
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

            buffer->updateDataPart(shadow.data() + range.begin, range.begin, part);
            spent += part;

            if (part < length)
            {
                range.begin += part;
                break;
            }

            ++consumed;
        }

        dirtyRanges.erase(dirtyRanges.begin(), dirtyRanges.begin() + static_cast<std::ptrdiff_t>(consumed));

        if (flushedBytes != nullptr)
        {
            *flushedBytes += spent;
        }
    }

    // 还没上传的脏字节数（用于观察预算是否足够）
    auto dirtyBytes() const -> uint64_t
    {
        uint64_t total = 0;

        for (const auto &range : dirtyRanges)
        {
            total += range.end - range.begin;
        }

        return total;
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

    struct OMRendererSegBufDirtyRange
    {
        uint32_t begin;
        uint32_t end;
    };

    // 相邻/重叠的脏区间在 gap <= 16KB 时合并，既避免逐块上传调用过多，又不会把
    // 整块 buffer 都算作脏（旧的 [dirtyMin, dirtyMax] 方案在区块分散时会退化成全部上传）。
    static constexpr uint32_t dirtyMergeGap = 16 * 1024;
    std::vector<OMRendererSegBufDirtyRange> dirtyRanges;
    uint32_t frameCounter = 0;
    std::vector<std::pair<OMRendererBuffer *, uint32_t>> retiredBuffers;

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

        // 旧 GPU 缓冲可能仍被飞行中的帧引用：延迟若干帧再释放
        retiredBuffers.push_back({buffer, frameCounter});
        buffer = newBuffer;

        freeBlocks.push_back({totalSize, totalSize});

        totalSize = newTotal;

        normalizeFreeBlocks();
    }
};

} // namespace openminecraft::renderer::common::wrap

#endif
