#include "boost/asio.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/impl/read.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace openminecraft;
using namespace boost::asio;

char *buf = new char[65536];

int readVarInt(ip::tcp::socket &socket)
{
    int value = 0;
    int position = 0;

    while (true)
    {
        uint8_t cb;
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

int main(int argc, char **argv)
{
    log::OMLogger logger("Network Test");
    logger.info("test!");

    io_context io;
    ip::tcp::socket socket(io);
    ip::tcp::resolver reso(io);
    auto temp = reso.resolve(argv[1], argv[2]);
    connect(socket, temp);

    auto timestmp = (uint64_t)time(nullptr);

    std::ostringstream payld;
    // packet 1: Handshake
    payld << (char)16;
    payld << (char)0x00;
    payld << (char)0b10000101 << (char)0b00000110;
    payld << (char)9;
    payld.write("localhost", 9);
    payld << (char)0b10000001 << (char)0b00111100;
    payld << (char)1;
    // packet 2: Fetch metadata
    payld << (char)1;
    payld << (char)0x00;
    // packet 3: ping request
    payld << (char)9;
    payld << (char)0x01;
    payld.write((char *)&timestmp, sizeof(uint64_t));

    write(socket, buffer(payld.str().c_str(), payld.str().size()));
    logger.info("connected to the Minecraft server!");

    std::ofstream of("server.dat");

    while (true)
    {
        try
        {
            auto length = readVarInt(socket) - 1;
            auto lcnst = length;
            uint8_t id;
            socket.read_some(buffer(&id, 1));
            while (length > 0)
            {
                auto l = socket.read_some(buffer(buf, length));
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

    return 0;
}
