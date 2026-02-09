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
#include <ctime>
#include <fstream>
#include <iostream>

using namespace openminecraft;
using namespace boost::asio;

char *buf = new char[65536];

int main()
{
    log::OMLogger logger("Network Test");
    logger.info("test!");

    io_context io;
    ip::tcp::socket socket(io);
    ip::tcp::resolver reso(io);
    auto temp = reso.resolve("localhost", "25566");
    for (auto i = temp.begin(); i != temp.end(); ++i)
    {
        logger.info("{}", (*i).host_name());
    }
    connect(socket, temp);
    unsigned char payload[] = {16, 0x00, 0b10000101, 0b00000110, // VarInt protocol version
                               9, 'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', 0b00000001,
                               0b10111100, // String + UShort
                                           // server ip/port
                               1,          // Enum status
                               1, 0x00, 9, 0x01, 0, 0, 0, 0, 0, 0, 0, 0};

    write(socket, buffer(payload, sizeof(payload)));
    logger.info("connected to the Minecraft server!");

    std::ofstream of("server.dat");

    while (true)
    {
        try
        {
            auto l = socket.read_some(buffer(buf, 65536));
            of.write(buf, l);
            logger.info("read {} bytes!", l);
            of.flush();
        }
        catch (boost::wrapexcept<boost::system::system_error> &e)
        {
            of.close();
            throw e;
        }
    }

    return 0;
}
