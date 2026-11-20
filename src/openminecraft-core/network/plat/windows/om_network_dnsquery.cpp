#include "openminecraft/network/om_network_dnsquery.hpp"

#include "openminecraft/log/om_log_common.hpp"
#include <vector>
#include <windows.h>
#include <windns.h>

namespace openminecraft::network
{
log::OMLogger logger("DNS Query");
auto queryDns(std::string n) -> std::vector<OMNetworkDnsResult>
{
    PDNS_RECORD ppQueryResultsSet = NULL;

    DNS_STATUS status = DnsQuery(n.c_str(), DNS_TYPE_SRV, DNS_QUERY_BYPASS_CACHE, NULL, &ppQueryResultsSet, NULL);

    if (status != ERROR_SUCCESS)
    {
        logger.warn("DnsQuery {} failed with error code: {}", n, status);
        DnsRecordListFree(ppQueryResultsSet, DnsFreeRecordList);
        return {};
    }

    std::vector<OMNetworkDnsResult> result;
    for (PDNS_RECORD pRecord = ppQueryResultsSet; pRecord != NULL; pRecord = pRecord->pNext)
    {
        if (pRecord->wType == DNS_TYPE_SRV)
        {
            result.push_back({pRecord->Data.SRV.pNameTarget, pRecord->Data.SRV.wPort});
        }
    }

    DnsRecordListFree(ppQueryResultsSet, DnsFreeRecordList);
    return result;
}
} // namespace openminecraft::network
