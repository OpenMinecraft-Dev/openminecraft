#include "boost/asio.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/impl/read.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace openminecraft;
using namespace boost::asio;

char *buf = new char[65536];

auto readVarInt(ip::tcp::socket &socket) -> int
{
    int value = 0;
    int position = 0;

    while (true)
    {
        uint8_t cb = 0;
        socket.read_some(buffer(&cb, 1));
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
    logger.info("test! {} {}", argv[1], argv[2]);

    io_context io;
    ip::tcp::socket socket(io);
    ip::tcp::resolver reso(io);
    auto temp = reso.resolve(argv[1], argv[2]);
    connect(socket, temp);

    auto timestmp = static_cast<uint64_t>(time(nullptr));

    std::ostringstream payld;
    // packet 1: Handshake
    payld << static_cast<char>(16);
    payld << static_cast<char>(0x00);
    payld << static_cast<char>(0b10000101) << static_cast<char>(0b00000110);
    payld << static_cast<char>(9);
    payld.write("localhost", 9);
    payld << static_cast<char>(0b10000001) << static_cast<char>(0b00111100);
    payld << static_cast<char>(1);
    // packet 2: Fetch metadata
    payld << static_cast<char>(1);
    payld << static_cast<char>(0x00);
    // packet 3: ping request
    /*payld << static_cast<char>(9);
    payld << static_cast<char>(0x01);
    payld.write(reinterpret_cast<char *>(&timestmp), sizeof(uint64_t));*/

    logger.info("connected to the Minecraft server!, timestamp {:016x}", timestmp);
    write(socket, buffer(payld.str().c_str(), payld.str().size()));

    std::ofstream of("server.dat");

    while (true)
    {
        try
        {
            auto length = readVarInt(socket) - 1;
            auto lcnst = length;
            uint8_t id = 0;
            socket.read_some(buffer(&id, 1));
            while (length > 0)
            {
                auto l = socket.read_some(buffer(buf, length));
                logger.debug("{} bytes", l);
                of.write(buf, l);
                length -= l;
            }
            logger.info("read packet 0x{:02x}, length {}", id, lcnst);
            of.flush();
        }
        catch (boost::wrapexcept<boost::system::system_error> &e)
        {
            of.close();
            logger.info("connection closed");
            socket.close();
            break;
        }
    }

    of.close();

    return 0;
}
