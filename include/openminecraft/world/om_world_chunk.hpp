#ifndef OM_WORLD_CHUNK_HPP
#define OM_WORLD_CHUNK_HPP

#include "glm/glm.hpp"
#include <atomic>
#include <cstdint>
#include <functional>
#include <iterator>
#include <utility>
#include <array>
#include <vector>
namespace openminecraft::world
{
template <int Cs> class OMChunk
{
  public:
    OMChunk(int64_t chunkx, int64_t chunky, int64_t chunkz) : chunkx(chunkx), chunky(chunky), chunkz(chunkz)
    {
    }

    // std::atomic members delete implicit copy/move constructors, so implement them explicitly
    OMChunk(const OMChunk &other)
        : chunkx(other.chunkx), chunky(other.chunky), chunkz(other.chunkz), visible(other.visible.load()),
          blocks(other.blocks), nonAirCount(other.nonAirCount.load()), dirty(other.dirty.load())
    {
    }

    auto operator=(const OMChunk &other) -> OMChunk &
    {
        chunkx = other.chunkx;
        chunky = other.chunky;
        chunkz = other.chunkz;
        blocks = other.blocks;
        visible.store(other.visible.load());
        nonAirCount.store(other.nonAirCount.load());
        dirty.store(other.dirty.load());
        return *this;
    }

    OMChunk(OMChunk &&other) noexcept
        : chunkx(other.chunkx), chunky(other.chunky), chunkz(other.chunkz), visible(other.visible.load()),
          blocks(std::move(other.blocks)), nonAirCount(other.nonAirCount.load()), dirty(other.dirty.load())
    {
    }

    auto operator=(OMChunk &&other) noexcept -> OMChunk &
    {
        chunkx = other.chunkx;
        chunky = other.chunky;
        chunkz = other.chunkz;
        blocks = std::move(other.blocks);
        visible.store(other.visible.load());
        nonAirCount.store(other.nonAirCount.load());
        dirty.store(other.dirty.load());
        return *this;
    }
    ~OMChunk()
    {
    }

    // Air block id of this chunk (set by the loader, used to count non-air blocks)
    void setAirId(uint32_t id)
    {
        airId = id;
    }

    // Fill the whole chunk with one block state (much faster than per-block setBlock)
    void fill(uint32_t blockState)
    {
        blocks.fill(blockState);
        // Mark all-air chunks as empty: compiler/renderer can skip them entirely, using no buffer space
        nonAirCount.store(blockState == airId ? 0 : static_cast<uint32_t>(Cs * Cs * Cs));
        dirty = true;
    }

    void setBlock(int x, int y, int z, uint32_t blockState)
    {
        auto &slot = blocks[y * Cs * Cs + x * Cs + z];

        if (slot == airId && blockState != airId)
        {
            ++nonAirCount;
        }
        else if (slot != airId && blockState == airId)
        {
            --nonAirCount;
        }

        slot = blockState;
        dirty = true;
    }

    // No non-air blocks in this chunk (the loader can tag it as empty directly)
    [[nodiscard]] auto isEmpty() const -> bool
    {
        return nonAirCount.load(std::memory_order_relaxed) == 0;
    }
    auto getBlock(int x, int y, int z) -> uint32_t
    {
        return blocks[y * Cs * Cs + x * Cs + z];
    }

    void solveDirty()
    {
        dirty.store(false);
    }

    auto isDirty() const -> bool
    {
        return dirty.load();
    }

    void markDirty()
    {
        dirty.store(true);
    }

    class const_iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<glm::ivec3, uint32_t>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type *;
        using reference = const value_type &;

        const_iterator(const OMChunk *chunk, size_t index) : chunk(chunk), index(index)
        {
        }

        auto operator++() -> const_iterator &
        {
            ++index;
            return *this;
        }

        auto operator*() const -> value_type
        {
            int x = (index / Cs) % Cs;
            int y = index / (Cs * Cs);
            int z = index % Cs;
            uint32_t block = chunk->blocks[index];
            return {{x, y, z}, block};
        }

        auto operator==(const const_iterator &other) const -> bool
        {
            return chunk == other.chunk && index == other.index;
        }
        auto operator!=(const const_iterator &other) const -> bool
        {
            return !(*this == other);
        }

        std::vector<uint32_t> extras = {};

      private:
        const OMChunk *chunk;
        size_t index;
    };
    auto begin() const -> const_iterator const
    {
        return const_iterator(this, 0);
    }
    auto end() const -> const_iterator const
    {
        return const_iterator(this, Cs * Cs * Cs);
    }

    [[nodiscard]] auto exists(int x, int y, int z) const -> bool
    {
        if (x < 0 || y < 0 || z < 0 || x >= Cs || y >= Cs || z >= Cs)
            return false;
        return blocks[y * Cs * Cs + x * Cs + z] != 0;
    }

    [[nodiscard]] auto fetch(int x, int y, int z) const -> uint32_t
    {
        if (x < 0 || y < 0 || z < 0 || x >= Cs || y >= Cs || z >= Cs)
            return 0;
        return blocks[y * Cs * Cs + x * Cs + z];
    }

    int64_t chunkx, chunky, chunkz;

    // Newly loaded chunks are invisible by default: avoids being treated as "previously visible" on the first cull,
    // which would take the hideFrames delayed-unload path (keeps rendering 120 frames and unloads only 32 per frame).
    std::atomic<bool> visible{false};
    // Non-air block count: 0 means an empty chunk; compiling and rendering can be skipped
    std::atomic<uint32_t> nonAirCount{0};
    uint32_t airId = 0;

  private:
    std::array<uint32_t, Cs * Cs * Cs> blocks = {};
    std::atomic<bool> dirty{true};
};
} // namespace openminecraft::world

#endif