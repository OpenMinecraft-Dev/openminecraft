#ifndef OM_RENDERER_SEGBUF_HPP
#define OM_RENDERER_SEGBUF_HPP

#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/om_renderer_layer.hpp"
#include <iostream>
#include <list>
#include <vector>
#include <algorithm>
#include <cmath>

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
        auto temp = std::malloc(totalSize);
        memset(temp, 0x00, totalSize);
        buffer->updateData(temp);
        std::free(temp);
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

        auto temp = std::malloc(block.length);
        memset(temp, 0x00, block.length);
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

    // 把传入的块列表按地址顺序挤到缓冲区头部（使用 GPU 侧的 moveDataPart 搬移），
    // 返回搬移后的块列表（offset 已更新），同时重建空闲链表，
    // 使 usedSize() 精确等于真实数据量。
    // 把传入的块搬进现有空闲区（绝不覆盖仍在使用中的数据）：
    //   1. 按 offset 升序处理，逐个在 freeBlocks 里找足够大的洞；
    //   2. 只往更低地址搬（消除高水位），搬完把旧区间清零归还空闲链表；
    //   3. 找不到合适的洞就保持原位。
    // 返回搬移后的块列表（offset 已更新）。
    auto compact(std::list<OMRendererSegBufBlock> blocks) -> std::list<OMRendererSegBufBlock>
    {
        blocks.sort(
            [](const OMRendererSegBufBlock &a, const OMRendererSegBufBlock &b) { return a.offset < b.offset; });

        std::list<OMRendererSegBufBlock> result;

        for (auto &block : blocks)
        {
            if (block.length == 0)
            {
                continue;
            }

            // 在空闲链表中找第一个足够大且位于当前块之前的洞
            auto it = freeBlocks.begin();

            while (it != freeBlocks.end() && (it->length < block.length || it->offset >= block.offset))
            {
                ++it;
            }

            if (it == freeBlocks.end())
            {
                result.push_back(block); // 没有可用空洞，保持原位
                continue;
            }

            // 从洞里切出目标区间（slice 会把洞的 offset 前移、length 减少）
            auto target = it->slice(block.length);

            if (it->length == 0)
            {
                freeBlocks.erase(it);
            }

            buffer->moveDataPart(target.offset, block.offset, block.length);

            // 旧区间清零并归还空闲链表（内部会排序 + 合并相邻洞）
            deallocate(block);

            block.offset = target.offset;
            result.push_back(block);
        }

        return result;
    }

    void update(OMRendererSegBufBlock block, const void *data)
    {
        buffer->updateDataPart(const_cast<void *>(data), block.offset, block.length);
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

    void expand()
    {
        uint32_t newTotal = totalSize * 2;
        OMRendererBuffer *newBuffer = renderer->allocateBuffer(InstanceData, newTotal);
        buffer->copyTo(newBuffer);
        auto temp = std::malloc(totalSize);
        memset(temp, 0x00, totalSize);
        newBuffer->updateDataPart(temp, totalSize, totalSize);
        std::free(temp);
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
