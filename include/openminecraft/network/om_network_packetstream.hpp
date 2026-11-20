#ifndef OM_NETWORK_PACKETSTREAM_HPP
#define OM_NETWORK_PACKETSTREAM_HPP

#include "openminecraft/network/om_network_packet.hpp"
#include <istream>
#include <streambuf>
#include <cstring>
#include <algorithm>

namespace openminecraft::network
{

class OMNetworkPacketStreamBuf : public std::streambuf
{
  public:
    explicit OMNetworkPacketStreamBuf(OMNetworkPacket &packet) : packet(packet)
    {
        syncFromPacket();
    }

  protected:
    auto underflow() -> int_type override
    {
        if (gptr() < egptr())
        {
            return traits_type::to_int_type(*gptr());
        }

        syncFromPacket();
        if (gptr() >= egptr())
        {
            return traits_type::eof();
        }
        return traits_type::to_int_type(*gptr());
    }

    auto uflow() -> int_type override
    {
        int_type c = underflow();
        if (c != traits_type::eof())
        {
            gbump(1);
            syncToPacket();
        }
        return c;
    }

    auto xsgetn(char *s, std::streamsize n) -> std::streamsize override
    {
        std::streamsize total = 0;

        while (total < n)
        {
            if (gptr() >= egptr())
            {
                syncFromPacket();
                if (gptr() >= egptr())
                {
                    break;
                }
            }

            std::streamsize avail = egptr() - gptr();
            std::streamsize take = std::min(avail, n - total);

            std::memcpy(s + total, gptr(), static_cast<size_t>(take));
            gbump(static_cast<int>(take));
            total += take;
        }

        syncToPacket();
        return total;
    }

    auto seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) -> pos_type override
    {
        if (!(which & std::ios_base::in))
        {
            return {off_type(-1)};
        }

        syncToPacket();

        off_type newOffset = 0;
        if (dir == std::ios_base::beg)
        {
            newOffset = off;
        }
        else if (dir == std::ios_base::cur)
        {
            newOffset = static_cast<off_type>(packet.offset) + off;
        }
        else if (dir == std::ios_base::end)
        {
            newOffset = static_cast<off_type>(packet.buffer.size()) + off;
        }

        if (newOffset < 0 || newOffset > static_cast<off_type>(packet.buffer.size()))
        {
            return pos_type(off_type(-1));
        }

        packet.offset = static_cast<uint64_t>(newOffset);
        syncFromPacket();
        return {newOffset};
    }

  private:
    void syncFromPacket()
    {
        baseOffset = packet.offset;

        auto &buf = packet.buffer;
        if (baseOffset < buf.size())
        {
            char *base = reinterpret_cast<char *>(buf.data() + baseOffset);
            char *end = base + (buf.size() - baseOffset);
            setg(base, base, end);
        }
        else
        {
            setg(nullptr, nullptr, nullptr);
        }
    }

    void syncToPacket()
    {
        if (eback() == nullptr)
        {
            return;
        }
        packet.offset = baseOffset + static_cast<uint64_t>(gptr() - eback());
    }

    OMNetworkPacket &packet;
    uint64_t baseOffset = 0;
};

class OMNetworkPacketIStream : public std::istream
{
  public:
    OMNetworkPacketIStream(OMNetworkPacket &packet) : std::istream(&buf), buf(packet)
    {
        exceptions(std::ios::badbit);
    }

  private:
    OMNetworkPacketStreamBuf buf;
};

} // namespace openminecraft::network

#endif