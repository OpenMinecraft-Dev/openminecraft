#ifndef OM_WORLD_CHUNKMANAGER_HPP
#define OM_WORLD_CHUNKMANAGER_HPP

#include "openminecraft/world/om_world_chunk.hpp"
#include <atomic>
#include <functional>
#include <iostream>
#include <mutex>
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

    void loadChunk(OMChunk<Cs> &chunk)
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        chunk.markDirty();
        OMChunkIndex idx{chunk.chunkx, chunk.chunky, chunk.chunkz};

        int slot;
        if (emptySlots.empty())
        {
            slot = chunks.size();
            chunks.emplace_back();
        }
        else
        {
            slot = emptySlots.back();
            emptySlots.pop_back();
        }
        chunks[slot].emplace(std::move(chunk));
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
        std::lock_guard<std::mutex> lock(chunkMutex);
        auto it = chunkMap.find(idx);
        if (it == chunkMap.end())
            return;

        int slot = it->second;
        chunkMap.erase(it);
        chunks[slot] = std::nullopt;
        emptySlots.push_back(slot);

        markIfExists({idx.x + 1, idx.y, idx.z});
        markIfExists({idx.x - 1, idx.y, idx.z});
        markIfExists({idx.x, idx.y + 1, idx.z});
        markIfExists({idx.x, idx.y - 1, idx.z});
        markIfExists({idx.x, idx.y, idx.z + 1});
        markIfExists({idx.x, idx.y, idx.z - 1});
    }

    auto getChunk(const OMChunkIndex &idx) const -> const OMChunk<Cs> &
    {
        auto it = chunkMap.find(idx);
        if (it == chunkMap.end())
            throw std::runtime_error("Chunk not loaded");
        return chunks[it->second].value();
    }

    auto getChunk(int i) const -> const std::optional<OMChunk<Cs>> &
    {
        return chunks[i];
    }

    auto getChunkMutable(const OMChunkIndex &idx) -> OMChunk<Cs> &
    {
        auto it = chunkMap.find(idx);
        if (it == chunkMap.end())
            throw std::runtime_error("Chunk not loaded");

        markIfExists({idx.x + 1, idx.y, idx.z});
        markIfExists({idx.x - 1, idx.y, idx.z});
        markIfExists({idx.x, idx.y + 1, idx.z});
        markIfExists({idx.x, idx.y - 1, idx.z});
        markIfExists({idx.x, idx.y, idx.z + 1});
        markIfExists({idx.x, idx.y, idx.z - 1});

        return chunks[it->second].value();
    }

    void markIfExists(OMChunkIndex idx)
    {
        if (chunkMap.count(idx))
        {
            chunks[chunkMap[idx]]->markDirty();
        }
    }

    auto chunkLoaded(const OMChunkIndex &idx) -> bool
    {
        return chunkMap.count(idx);
    }

    template <typename Func> void withChunks(Func &&func)
    {
        std::lock_guard g(chunkMutex);
        func(chunks);
    }

    auto numChunks() -> int
    {
        return chunks.size();
    }

  private:
    std::vector<std::optional<OMChunk<Cs>>> chunks;
    std::unordered_map<OMChunkIndex, int> chunkMap;
    std::vector<int> emptySlots;
    std::mutex chunkMutex;
};
} // namespace openminecraft::world

#endif