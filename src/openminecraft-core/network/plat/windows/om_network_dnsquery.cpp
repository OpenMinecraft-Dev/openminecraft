#include "openminecraft/network/om_network_dnsquery.hpp"

#include "openminecraft/log/om_log_common.hpp"
#include <vector>

namespace openminecraft::network
{
log::OMLogger logger("DNS Query");
auto queryDns(std::string n) -> std::vector<OMNetworkDnsResult>
{
    return {};
}
} // namespace openminecraft::network
