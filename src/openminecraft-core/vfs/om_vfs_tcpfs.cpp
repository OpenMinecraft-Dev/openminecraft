#include "openminecraft/vfs/om_vfs_tcpfs.hpp"
#include <iomanip>
#include <iostream>
#include <istream>
#include <fstream>
#include <memory>

#include "boost/asio.hpp"
#include <boost/asio/connect.hpp>
#include <boost/asio/impl/read.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/system/detail/error_code.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <stdexcept>

using namespace boost::asio;

namespace openminecraft::vfs
{
class OMFsProviderTcpBuf : public std::streambuf
{
  public:
    OMFsProviderTcpBuf(OMFsProviderTcp *p) : provider(p)
    {
    }

  protected:
    auto xsgetn(char *s, std::streamsize n) -> std::streamsize override
    {
        std::streamsize total = 0;
        while (total < n)
        {
            boost::system::error_code ec;
            auto r = provider->socket.read_some(buffer(s + total, n - total), ec);

            if (ec.failed())
            {
                if (total > 0)
                    return total;
                throw std::logic_error("connection lost: " + ec.message());
            }

            if (r == 0)
            {
                if (total > 0)
                    return total;
                throw std::logic_error("connection closed by peer");
            }

            total += r;
        }
        return total;
    }
    auto xsputn(const char *s, std::streamsize n) -> std::streamsize override
    {
        boost::system::error_code ec;
        auto r = provider->socket.write_some(buffer(s, n), ec);
        if (ec.failed())
        {
            throw std::logic_error("connection lost: " + ec.message());
        }
        return r;
    }
    auto showmanyc() -> std::streamsize override
    {
        boost::system::error_code ec;
        auto r = provider->socket.available(ec);
        if (ec.failed())
        {
            throw std::logic_error("connection lost: " + ec.message());
        }
        return r;
    }

    OMFsProviderTcp *provider;
};

class OMFsProviderTcpIStream : public std::istream
{
  public:
    OMFsProviderTcpIStream(OMFsProviderTcp *p) : std::istream(nullptr), buf(p)
    {
        rdbuf(&buf);
    }

  private:
    OMFsProviderTcpBuf buf;
};

class OMFsProviderTcpOStream : public std::ostream
{
  public:
    OMFsProviderTcpOStream(OMFsProviderTcp *p) : std::ostream(nullptr), buf(p)
    {
        rdbuf(&buf);
        exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);
    }

  private:
    OMFsProviderTcpBuf buf;
};

OMFsProviderTcp::~OMFsProviderTcp()
{
    if (connected)
    {
        socket.close();
    }
}

auto OMFsProviderTcp::read(std::string proc) -> std::shared_ptr<std::istream>
{
    connect();
    return std::make_shared<OMFsProviderTcpIStream>(this);
}
auto OMFsProviderTcp::write(std::string proc) -> std::shared_ptr<std::ostream>
{
    connect();
    return std::make_shared<OMFsProviderTcpOStream>(this);
}

void OMFsProviderTcp::connect()
{
    if (connected)
    {
        return;
    }

    ip::tcp::resolver resolv(context);
    auto target = resolv.resolve(host, port);
    socket.connect(*target.begin());

    connected = true;
}
} // namespace openminecraft::vfs