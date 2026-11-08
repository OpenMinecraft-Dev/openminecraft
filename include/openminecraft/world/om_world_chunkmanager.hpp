#ifndef OM_WORLD_CHUNKMANAGER_HPP
#define OM_WORLD_CHUNKMANAGER_HPP

#include "openminecraft/world/om_world_chunk.hpp"
#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <optional>
#include <vector>
namespace openminecraft::world
{
struct OMChunkIndex
{
    int64_t x, y, z;

    auto operator==(const OMChunkIndex &c) const -> bool
    {
        return x == c.x && y == c.y && z == c.z;
    }
};
} // namespace openminecraft::world

namespace std
{
template <> struct hash<openminecraft::world::OMChunkIndex>
{
    size_t operator()(const openminecraft::world::OMChunkIndex &c) const noexcept
    {
        uint64_t hash = std::hash<int64_t>{}(c.x);
        hash ^= std::hash<int64_t>{}(c.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        hash ^= std::hash<int64_t>{}(c.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
    }
};
} // namespace std

namespace openminecraft::world
{
template <int Cs> class OMChunkManager
{
  public:
    OMChunkManager() = default;

    // Load/unload version: the renderer uses it to detect chunk loading and avoid pointless culling recomputation
    auto loadVersion() const -> uint64_t
    {
        return version.load(std::memory_order_relaxed);
    }

    void loadChunk(OMChunk<Cs> &chunk)
    {
        std::unique_lock<std::shared_mutex> lock(chunkMutex);
        version.fetch_add(1, std::memory_order_relaxed);
        chunk.markDirty();
        OMChunkIndex idx{chunk.chunkx, chunk.chunky, chunk.chunkz};

        int slot;
        auto existing = chunkMap.find(idx);
        if (existing != chunkMap.end())
        {
            // Duplicate coordinates: overwrite the original slot to avoid leaking ghost chunks
            slot = existing->second;
        }
        else if (emptySlots.empty())
        {
            slot = static_cast<int>(chunks.size());
            chunks.emplace_back();
        }
        else
        {
            slot = emptySlots.back();
            emptySlots.pop_back();
        }
        chunks[slot] = std::make_shared<OMChunk<Cs>>(std::move(chunk));
        chunkMap[idx] = slot;

        markIfExists({idx.x + 1, idx.y, idx.z});
        markIfExists({idx.x - 1, idx.y, idx.z});
        markIfExists({idx.x, idx.y + 1, idx.z});
        markIfExists({idx.x, idx.y - 1, idx.z});
        markIfExists({idx.x, idx.y, idx.z + 1});
        markIfExists({idx.x, idx.y, idx.z - 1});
    }

    void unloadChunk(const OMChunkIndex &idx)
    {
        std::unique_lock<std::shared_mutex> lock(chunkMutex);
        version.fetch_add(1, std::memory_order_relaxed);
        auto it = chunkMap.find(idx);
        if (it == chunkMap.end())
            return;

        int slot = it->second;
        chunkMap.erase(it);
        chunks[slot].reset();
        emptySlots.push_back(slot);

        markIfExists({idx.x + 1, idx.y, idx.z});
        markIfExists({idx.x - 1, idx.y, idx.z});
        markIfExists({idx.x, idx.y + 1, idx.z});
        markIfExists({idx.x, idx.y - 1, idx.z});
        markIfExists({idx.x, idx.y, idx.z + 1});
        markIfExists({idx.x, idx.y, idx.z - 1});
    }

    auto getChunk(const OMChunkIndex &idx) const -> std::shared_ptr<const OMChunk<Cs>>
    {
        std::shared_lock<std::shared_mutex> lock(chunkMutex);
        auto it = chunkMap.find(idx);
        if (it == chunkMap.end())
            throw std::runtime_error("Chunk not loaded");
        return chunks[it->second];
    }

    // Non-throwing variant (used by worker threads for neighbor queries)
    auto tryGetChunk(const OMChunkIndex &idx) const -> std::shared_ptr<const OMChunk<Cs>>
    {
        std::shared_lock<std::shared_mutex> lock(chunkMutex);
        auto it = chunkMap.find(idx);
        if (it == chunkMap.end())
            return nullptr;
        return chunks[it->second];
    }

    auto getChunk(int i) const -> std::shared_ptr<OMChunk<Cs>>
    {
        std::shared_lock<std::shared_mutex> lock(chunkMutex);
        if (i < 0 || static_cast<size_t>(i) >= chunks.size())
            return nullptr;
        return chunks[i];
    }

    auto getChunkMutable(const OMChunkIndex &idx) -> std::shared_ptr<OMChunk<Cs>>
    {
        std::unique_lock<std::shared_mutex> lock(chunkMutex);
        auto it = chunkMap.find(idx);
        if (it == chunkMap.end())
            throw std::runtime_error("Chunk not loaded");

        markIfExists({idx.x + 1, idx.y, idx.z});
        markIfExists({idx.x - 1, idx.y, idx.z});
        markIfExists({idx.x, idx.y + 1, idx.z});
        markIfExists({idx.x, idx.y - 1, idx.z});
        markIfExists({idx.x, idx.y, idx.z + 1});
        markIfExists({idx.x, idx.y, idx.z - 1});

        return chunks[it->second];
    }

    auto chunkLoaded(const OMChunkIndex &idx) -> bool
    {
        std::shared_lock<std::shared_mutex> lock(chunkMutex);
        return chunkMap.count(idx) > 0;
    }

    template <typename Func> void withChunks(Func &&func)
    {
        std::shared_lock<std::shared_mutex> lock(chunkMutex);
        func(chunks);
    }

    // Expose both the coordinate index map and the slot array (occlusion-culling BFS needs neighbor lookup by coordinate)
    template <typename Func> void withChunkMap(Func &&func)
    {
        std::shared_lock<std::shared_mutex> lock(chunkMutex);
        func(chunkMap, chunks);
    }

    // Current slot count (including empty slots)
    auto chunkSlotCount() -> int
    {
        std::shared_lock<std::shared_mutex> lock(chunkMutex);
        return static_cast<int>(chunks.size());
    }

    auto numChunks() -> int
    {
        std::shared_lock<std::shared_mutex> lock(chunkMutex);
        return static_cast<int>(chunks.size());
    }

  private:
    // Only call while holding the lock
    void markIfExists(OMChunkIndex idx)
    {
        auto it = chunkMap.find(idx);
        if (it != chunkMap.end())
        {
            chunks[it->second]->markDirty();
        }
    }

    // Slots hold shared_ptr: elements stay stable and old holders remain safe after unload
    std::atomic<uint64_t> version{0};
    std::vector<std::shared_ptr<OMChunk<Cs>>> chunks;
    std::unordered_map<OMChunkIndex, int> chunkMap;
    std::vector<int> emptySlots;
    mutable std::shared_mutex chunkMutex;
};
} // namespace openminecraft::world

#endif