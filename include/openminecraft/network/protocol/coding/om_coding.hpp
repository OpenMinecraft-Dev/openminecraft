#ifndef OM_CODING_HPP
#define OM_CODING_HPP

#include "openminecraft/network/om_network_packet.hpp"
#include <istream>
#include <memory>
#include <utility>
namespace openminecraft::network::protocol
{
class OMCodingBase
{
  public:
    OMCodingBase(std::shared_ptr<std::istream> in, std::shared_ptr<std::ostream> out)
        : in(std::move(in)), out(std::move(out))
    {
    }

    virtual void write(OMNetworkPacket) = 0;
    virtual auto read() -> OMNetworkPacket = 0;

  protected:
    std::shared_ptr<std::istream> in;
    std::shared_ptr<std::ostream> out;
};
} // namespace openminecraft::network::protocol

#endif