#ifndef OM_RENDERER_VOXEL_LAYER_HPP
#define OM_RENDERER_VOXEL_LAYER_HPP

#include "openminecraft/renderer/common/wrap/om_renderer_segbuf.hpp"
#include "openminecraft/renderer/common/wrap/voxel/om_renderer_voxel_objects.hpp"
#include <chrono>
#include <list>
#include <vector>

namespace openminecraft::renderer::common::wrap::voxel
{
template <typename It> class OMVoxelLayer
{
  public:
    OMVoxelLayer(OMRenderer *renderer, std::string tag)
    {
        voxelBuffer = new OMRendererSegBuf(renderer, std::move(tag), 1024 * 1024);
        chunkBlocks.resize(1);
    }
    ~OMVoxelLayer()
    {
        delete voxelBuffer;
    }

    inline void loadData(int i, std::vector<It> &cm, bool flush = false)
    {
        if (chunkBlocks.size() <= i)
        {
            chunkBlocks.resize(i + 1);
        }

        while (!chunkBlocks[i].empty())
        {
            liveBytes -= chunkBlocks[i].back().length;
            voxelBuffer->deallocate(chunkBlocks[i].back());
            chunkBlocks[i].pop_back();
        }

        while (!cm.empty())
        {
            auto blk = voxelBuffer->allocate(sizeof(It), cm.size() * sizeof(It), sizeof(It));
            voxelBuffer->update(blk, cm.data());
            cm.erase(cm.begin(), std::next(cm.begin(), blk.length / sizeof(It)));
            liveBytes += blk.length;
            chunkBlocks[i].push_back(blk);
        }

        if (flush)
        {
            voxelBuffer->flushDirty();
        }
    }

    auto merge()
    {
        auto mergeStart = std::chrono::steady_clock::now();

        for (auto &p : chunkBlocks)
        {
            p = voxelBuffer->compact(p);
        }
    }

    // Time-limited compaction: avoid compacting an entire layer at once and causing tens of ms of stutter
    auto mergeStepTimed(double budgetMs) -> void
    {
        if (chunkBlocks.empty())
        {
            return;
        }

        const auto start = std::chrono::steady_clock::now();
        size_t processed = 0;

        while (mergeCursor < chunkBlocks.size())
        {
            chunkBlocks[mergeCursor] = voxelBuffer->compact(chunkBlocks[mergeCursor]);
            ++mergeCursor;
            ++processed;

            if ((processed & 31) == 0 &&
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() >
                    budgetMs)
            {
                break;
            }
        }

        if (mergeCursor >= chunkBlocks.size())
        {
            mergeCursor = 0;
        }
    }

    auto mergeStep(size_t maxChunks) -> void
    {
        if (chunkBlocks.empty())
        {
            return;
        }

        size_t processed = 0;

        while (processed < maxChunks && mergeCursor < chunkBlocks.size())
        {
            chunkBlocks[mergeCursor] = voxelBuffer->compact(chunkBlocks[mergeCursor]);
            ++mergeCursor;
            ++processed;
        }

        if (mergeCursor >= chunkBlocks.size())
        {
            mergeCursor = 0;
        }
    }

    auto buf() -> OMRendererSegBuf *
    {
        return voxelBuffer;
    }


    // Actual live data size (used to judge fragmentation)
    auto liveBytesCount() const -> uint64_t
    {
        return liveBytes;
    }

  private:
    size_t mergeCursor = 0;
    uint64_t liveBytes = 0;
    std::vector<std::list<OMRendererSegBufBlock>> chunkBlocks;
    OMRendererSegBuf *voxelBuffer;
};

} // namespace openminecraft::renderer::common::wrap::voxel

#endif
