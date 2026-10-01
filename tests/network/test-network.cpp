#include "boost/system/system_error.hpp"
#include "boost/throw_exception.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/network/om_network_dnsquery.hpp"
#include "openminecraft/network/om_network_packet.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <istream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <thread>

using namespace openminecraft;

char *buf = new char[65536];

auto readVarInt(std::shared_ptr<std::istream> istr) -> int
{
    int value = 0;
    int position = 0;

    while (true)
    {
        char cb = 0;
        istr->read(&cb, 1);
        value |= (cb & 0x7f) << position;

        if ((cb & 0x80) == 0)
        {
            break;
        }

        position += 7;

        if (position >= 32)
        {
            throw std::runtime_error("invalid VarInt");
        }
    }

    return value;
}

auto main(int argc, char **argv) -> int
{
    log::OMLogger logger("Network Test");
    logger.info("try connect to {} {}", argv[1], argv[2]);

    auto res = network::queryDns("_minecraft._tcp.awa.kjmc.top");
    for (const auto &r : res)
    {
        logger.warn("{}:{}", r.target, r.port);
    }

    vfs::fsmountTcp(argv[1], argv[2], "/mcserver_conn");
    auto in = vfs::fsfetch("/mcserver_conn/connect");
    auto out = vfs::fswrite("/mcserver_conn/connect");

    auto timestmp = static_cast<uint64_t>(time(nullptr));

    std::ostringstream payld;
    // packet 1: Handshake
    auto pck = network::OMNetworkPacket()
                   .varInt(16)
                   .varInt(0x00)
                   .varInt(773)
                   .utf8WithLength("localhost")
                   .int16(25565)
                   .varInt(1);
    payld.write(reinterpret_cast<char *>(pck.data()), pck.datalen());
    // packet 2: Fetch metadata
    auto pck2 = network::OMNetworkPacket().varInt(1).varInt(0x00);
    payld.write(reinterpret_cast<char *>(pck2.data()), pck2.datalen());
    // packet 3: ping request
    payld << static_cast<char>(9);
    payld << static_cast<char>(0x01);
    payld.write(reinterpret_cast<char *>(&timestmp), sizeof(uint64_t));

    logger.info("connected to the Minecraft server!, timestamp {:016x}", timestmp);
    out->write(payld.str().c_str(), payld.str().size());

    std::ofstream of("server.dat");

    while (true)
    {
        try
        {
            auto length = readVarInt(in) - 1;
            auto lcnst = length;
            char id = 0;
            in->read(&id, 1);
            while (length > 0)
            {
                auto l = in->readsome(buf, length);
                if (l != 0)
                {
                    logger.debug("{} bytes", l);
                }
                of.write(buf, length);
                length -= l;
            }
            logger.info("read packet 0x{:02x}, length {}", id, lcnst);
            of.flush();
        }
        catch (std::logic_error &e)
        {
            of.close();
            logger.info("{}", e.what());
            vfs::fsumount("/mcserver_conn");
            break;
        }
    }

    of.close();

    return 0;
}
