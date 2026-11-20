#ifndef OM_VFS_TCPFS_HPP
#define OM_VFS_TCPFS_HPP
#include <utility>

#include "boost/asio/io_context.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
namespace openminecraft::vfs
{
class OMFsProviderTcp : public OMFsProvider
{
  public:
    OMFsProviderTcp(std::string host, std::string port) : host(std::move(host)), port(std::move(port)), socket(context)
    {
    }

    ~OMFsProviderTcp();

    auto read(std::string) -> std::shared_ptr<std::istream> override;
    auto write(std::string) -> std::shared_ptr<std::ostream> override;
    boost::asio::io_context context = {};
    boost::asio::ip::tcp::socket socket;

  private:
    void connect();

    bool connected = false;
    std::string host;
    std::string port;
};
} // namespace openminecraft::vfs
#endif