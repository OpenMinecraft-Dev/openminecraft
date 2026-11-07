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

    // std::atomic 成员会删除隐式的拷贝/移动构造，这里显式实现
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

    // 该区块的空气方块 id（加载器设置，用于统计非空气方块数）
    void setAirId(uint32_t id)
    {
        airId = id;
    }

    // 用同一个方块状态填充整个区块（比逐块 setBlock 快得多）
    void fill(uint32_t blockState)
    {
        blocks.fill(blockState);
        // 全空气区块标记为 empty：编译器/渲染器可以直接跳过，不占缓冲
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

    // 区块内没有任何非空气方块（加载器可直接打上 empty tag）
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

    // 新加载的区块默认不可见：避免第一次剔除时被当成"原本可见"，
    // 从而走 hideFrames 延迟卸载（会保留渲染 120 帧，且每帧只卸载 32 个）。
    std::atomic<bool> visible{false};
    // 非空气方块数量：为 0 即 empty chunk，编译与渲染都可以直接跳过
    std::atomic<uint32_t> nonAirCount{0};
    uint32_t airId = 0;

  private:
    std::array<uint32_t, Cs * Cs * Cs> blocks = {};
    std::atomic<bool> dirty{true};
};
} // namespace openminecraft::world

#endif