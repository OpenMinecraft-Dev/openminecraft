#ifndef OM_NETWORK_DNSQUERY_HPP
#define OM_NETWORK_DNSQUERY_HPP

#include <string>
#include <cstdint>
#include <vector>

namespace openminecraft::network
{
struct OMNetworkDnsResult
{
    std::string target;
    uint16_t port;
};

auto queryDns(std::string) -> std::vector<OMNetworkDnsResult>;
} // namespace openminecraft::network

#endif