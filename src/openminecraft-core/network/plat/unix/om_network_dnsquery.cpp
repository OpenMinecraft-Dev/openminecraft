#include "openminecraft/network/om_network_dnsquery.hpp"

#include "openminecraft/log/om_log_common.hpp"
#include "resolv.h"
#include <netdb.h>
#include <vector>

namespace openminecraft::network
{
log::OMLogger logger("DNS Query");
auto queryDns(std::string n) -> std::vector<OMNetworkDnsResult>
{
    res_init();

    unsigned char answer[65536];
    int len = res_query("_minecraft._tcp.awa.kjmc.top", ns_c_in, ns_t_srv, answer, sizeof(answer));
    if (len < 0)
    {
        logger.warn("DNS query failed: {}", hstrerror(h_errno));
        return {};
    }

    ns_msg handle;
    ns_initparse(answer, len, &handle);

    int count = ns_msg_count(handle, ns_s_an);
    std::vector<OMNetworkDnsResult> result;
    for (int i = 0; i < count; i++)
    {
        ns_rr rr;
        ns_parserr(&handle, ns_s_an, i, &rr);
        if (ns_rr_type(rr) == ns_t_srv)
        {
            const unsigned char *rd = ns_rr_rdata(rr);
            uint16_t priority = (rd[0] << 8) | rd[1];
            uint16_t weight = (rd[2] << 8) | rd[3];
            uint16_t port = (rd[4] << 8) | rd[5];

            char target[NS_MAXDNAME];
            if (dn_expand(answer, answer + len, rd + 6, target, sizeof(target)) < 0)
            {
                continue;
            }

            result.push_back({target, port});
        }
    }

    return result;
}
} // namespace openminecraft::network
