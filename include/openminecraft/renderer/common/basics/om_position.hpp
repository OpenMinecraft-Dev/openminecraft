#ifndef OM_POSITION_HPP
#define OM_POSITION_HPP

#include <glm/glm.hpp>
#include <cstdint>

namespace openminecraft::renderer::common::basics
{
template <int Cs, typename Cp, typename Lp> class OMPosition
{
  public:
    OMPosition(Cp cx, Cp cy, Cp cz, Lp lx, Lp ly, Lp lz)
        : chunkx(cx), chunky(cy), chunkz(cz), localx(lx), localy(ly), localz(lz)
    {
    }

    OMPosition(Cp cx, Cp cy, Cp cz) : chunkx(cx), chunky(cy), chunkz(cz), localx(0), localy(0), localz(0)
    {
    }

    OMPosition() : chunkx(0), chunky(0), chunkz(0), localx(0), localy(0), localz(0)
    {
    }
    OMPosition(const OMPosition &) = default;
    auto operator=(const OMPosition &) -> OMPosition & = default;

    OMPosition(glm::vec3 combinedPos)
        : chunkx(static_cast<Cp>(combinedPos.x / Cs)), chunky(static_cast<Cp>(combinedPos.y / Cs)),
          chunkz(static_cast<Cp>(combinedPos.z / Cs))
    {
        localx = static_cast<Lp>(combinedPos.x - chunkx * Cs);
        localy = static_cast<Lp>(combinedPos.y - chunky * Cs);
        localz = static_cast<Lp>(combinedPos.z - chunkz * Cs);
    }

    auto toCombinedPos() const -> glm::vec3
    {
        return glm::vec3(chunkx * Cs + localx, chunky * Cs + localy, chunkz * Cs + localz);
    }

    auto getChunkX() const -> Cp
    {
        return chunkx;
    }
    auto getChunkY() const -> Cp
    {
        return chunky;
    }
    auto getChunkZ() const -> Cp
    {
        return chunkz;
    }
    auto getLocalX() const -> Lp
    {
        return localx;
    }
    auto getLocalY() const -> Lp
    {
        return localy;
    }
    auto getLocalZ() const -> Lp
    {
        return localz;
    }

    auto operator+=(const glm::vec3 &delta) -> OMPosition &
    {
        localx += (Lp)delta.x;
        localy += (Lp)delta.y;
        localz += (Lp)delta.z;
        normalize();
        return *this;
    }

    auto operator+(const glm::vec3 &delta) const -> OMPosition
    {
        OMPosition result = *this;
        result += delta;
        return result;
    }

    friend auto operator+(const glm::vec3 &delta, const OMPosition &pos) -> OMPosition
    {
        return pos + delta;
    }

    auto operator-=(const glm::vec3 &delta) -> OMPosition &
    {
        localx -= (Lp)delta.x;
        localy -= (Lp)delta.y;
        localz -= (Lp)delta.z;
        normalize();
        return *this;
    }

    auto operator-(const glm::vec3 &delta) const -> OMPosition
    {
        OMPosition result = *this;
        result -= delta;
        return result;
    }

    auto operator-(const OMPosition &other) const -> glm::vec3
    {
        double dx = (double)(chunkx - other.chunkx) * Cs + ((double)localx - (double)other.localx);
        double dy = (double)(chunky - other.chunky) * Cs + ((double)localy - (double)other.localy);
        double dz = (double)(chunkz - other.chunkz) * Cs + ((double)localz - (double)other.localz);
        return {(float)dx, (float)dy, (float)dz};
    }

    Cp chunkx, chunky, chunkz;
    Lp localx, localy, localz;

  private:
    void normalize()
    {
        auto lx = (long double)localx;
        auto ly = (long double)localy;
        auto lz = (long double)localz;
        const auto cs = (long double)Cs;

        if (lx >= cs || lx < 0)
        {
            long double carry = std::floor(lx / cs);
            chunkx += (Cp)carry;
            lx -= carry * cs;
            if (lx < 0)
            {
                lx += cs;
                chunkx -= 1;
            }
            else if (lx >= cs)
            {
                lx -= cs;
                chunkx += 1;
            }
        }

        if (ly >= cs || ly < 0)
        {
            long double carry = std::floor(ly / cs);
            chunky += (Cp)carry;
            ly -= carry * cs;
            if (ly < 0)
            {
                ly += cs;
                chunky -= 1;
            }
            else if (ly >= cs)
            {
                ly -= cs;
                chunky += 1;
            }
        }

        if (lz >= cs || lz < 0)
        {
            long double carry = std::floor(lz / cs);
            chunkz += (Cp)carry;
            lz -= carry * cs;
            if (lz < 0)
            {
                lz += cs;
                chunkz -= 1;
            }
            else if (lz >= cs)
            {
                lz -= cs;
                chunkz += 1;
            }
        }

        localx = (Lp)lx;
        localy = (Lp)ly;
        localz = (Lp)lz;
    }
};
} // namespace openminecraft::renderer::common::basics

#endif
