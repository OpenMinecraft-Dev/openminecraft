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
    explicit OMNetworkPacketStreamBuf(OMNetworkPacket &packet) : packet_(packet)
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
            newOffset = static_cast<off_type>(packet_.offset) + off;
        }
        else if (dir == std::ios_base::end)
        {
            newOffset = static_cast<off_type>(packet_.buffer.size()) + off;
        }

        if (newOffset < 0 || newOffset > static_cast<off_type>(packet_.buffer.size()))
        {
            return pos_type(off_type(-1));
        }

        packet_.offset = static_cast<uint64_t>(newOffset);
        syncFromPacket();
        return {newOffset};
    }

  private:
    void syncFromPacket()
    {
        baseOffset_ = packet_.offset;

        auto &buf = packet_.buffer;
        if (baseOffset_ < buf.size())
        {
            char *base = reinterpret_cast<char *>(buf.data() + baseOffset_);
            char *end = base + (buf.size() - baseOffset_);
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
        packet_.offset = baseOffset_ + static_cast<uint64_t>(gptr() - eback());
    }

    OMNetworkPacket &packet_;
    uint64_t baseOffset_ = 0;
};

class OMNetworkPacketIStream : public std::istream
{
  public:
    OMNetworkPacketIStream(OMNetworkPacket &packet) : std::istream(&buf_), buf_(packet)
    {
        exceptions(std::ios::badbit);
    }

  private:
    OMNetworkPacketStreamBuf buf_;
};

} // namespace openminecraft::network

#endif