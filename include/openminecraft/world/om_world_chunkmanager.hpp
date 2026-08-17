#ifndef OM_WORLD_CHUNKMANAGER_HPP
#define OM_WORLD_CHUNKMANAGER_HPP

#include "openminecraft/world/om_world_chunk.hpp"
#include <atomic>
#include <functional>
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
    }

    void unloadChunk(const OMChunkIndex &idx)
    {
        std::lock_guard<std::mutex> lock(chunkMutex);
        auto it = chunkMap.find(idx);
        if (it == chunkMap.end())
            return;

        int slot = it->second;
        chunkMap.erase(it);
        chunks[slot].reset();
        emptySlots.push_back(slot);
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
        return chunks[it->second].value();
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