#ifndef OM_WORLD_CHUNK_HPP
#define OM_WORLD_CHUNK_HPP

#include "glm/glm.hpp"
#include <cstdint>
#include <iterator>
#include <utility>
namespace openminecraft::world
{
template <int Cs> class OMChunk
{
  public:
    OMChunk(int64_t chunkx, int64_t chunky, int64_t chunkz) : chunkx(chunkx), chunky(chunky), chunkz(chunkz)
    {
    }
    ~OMChunk()
    {
    }

    void setBlock(int x, int y, int z, uint32_t block)
    {
        blocks[y * Cs * Cs + x * Cs + z] = block;
    }
    auto getBlock(int x, int y, int z) -> uint32_t
    {
        return blocks[y * Cs * Cs + x * Cs + z];
    }

    class const_iterator
    {
      public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::pair<glm::ivec3, uint32_t>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type *;
        using reference = const value_type &;

        const_iterator(const OMChunk *chunk, size_t index) : chunk_(chunk), index_(index)
        {
        }

        auto operator++() -> const_iterator &
        {
            ++index_;
            return *this;
        }

        auto operator*() const -> value_type
        {
            int x = (index_ / Cs) % Cs;
            int y = index_ / (Cs * Cs);
            int z = index_ % Cs;
            uint32_t block = chunk_->blocks[index_];
            return {{x, y, z}, block};
        }

        auto operator==(const const_iterator &other) const -> bool
        {
            return chunk_ == other.chunk_ && index_ == other.index_;
        }
        auto operator!=(const const_iterator &other) const -> bool
        {
            return !(*this == other);
        }

      private:
        const OMChunk *chunk_;
        size_t index_;
    };
    auto begin() -> const_iterator const
    {
        return const_iterator(this, 0);
    }
    auto end() -> const_iterator const
    {
        return const_iterator(this, Cs * Cs * Cs);
    }

    auto exists(int x, int y, int z) -> bool
    {
        if (x < 0 || y < 0 || z < 0 || x >= Cs || y >= Cs || z >= Cs)
            return false;
        return blocks[y * Cs * Cs + x * Cs + z] != 0;
    }

    int64_t chunkx, chunky, chunkz;

  private:
    std::array<uint32_t, Cs * Cs * Cs> blocks = {};
};
} // namespace openminecraft::world

#endif