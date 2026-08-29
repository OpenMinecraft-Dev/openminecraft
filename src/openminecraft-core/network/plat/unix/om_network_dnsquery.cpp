#include "openminecraft/network/om_network_dnsquery.hpp"

#include "openminecraft/log/om_log_common.hpp"
#include "resolv.h"
#include <netdb.h>
#include <vector>

namespace openminecraft::network
{
log::OMLogger logger("DNS Query");
struct SRVRecord
{
    uint16_t priority;
    uint16_t weight;
    uint16_t port;
    std::string target;
};
static uint16_t readU16(const unsigned char *p)
{
    return (static_cast<uint16_t>(p[0]) << 8) | p[1];
}

static uint32_t readU32(const unsigned char *p)
{
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | p[3];
}

static int skipName(const unsigned char *msg, int offset)
{
    while (true)
    {
        uint8_t len = msg[offset];
        if (len == 0)
            return offset + 1;
        if ((len & 0xC0) == 0xC0)
            return offset + 2;
        offset += len + 1;
    }
}

static int expandName(const unsigned char *msg, int offset, std::string &name)
{
    name.clear();
    bool jumped = false;
    int nextOffset = -1;
    int safetyCounter = 0;

    while (true)
    {
        uint8_t len = msg[offset];
        if (len == 0)
        {
            if (!jumped)
                nextOffset = offset + 1;
            break;
        }
        if ((len & 0xC0) == 0xC0)
        {
            uint16_t ptr = ((len & 0x3F) << 8) | msg[offset + 1];
            if (!jumped)
                nextOffset = offset + 2;
            offset = ptr;
            jumped = true;
            if (++safetyCounter > 10)
                return -1;
            continue;
        }
        name.append(reinterpret_cast<const char *>(msg + offset + 1), len);
        name += '.';
        offset += len + 1;
    }
    if (!name.empty() && name.back() == '.')
        name.pop_back();
    return nextOffset;
}

auto parseSRVResponse(const unsigned char *answer, int len) -> std::vector<SRVRecord>
{
    std::vector<SRVRecord> result;
    if (len < 12)
        return result;

    uint16_t qdcount = readU16(answer + 4);
    uint16_t ancount = readU16(answer + 6);

    int offset = 12;
    for (int i = 0; i < qdcount; ++i)
    {
        offset = skipName(answer, offset);
        offset += 4;
    }

    for (int i = 0; i < ancount; ++i)
    {
        offset = skipName(answer, offset);

        uint16_t type = readU16(answer + offset);
        uint16_t cls = readU16(answer + offset + 2);
        uint32_t ttl = readU32(answer + offset + 4);
        uint16_t rdlen = readU16(answer + offset + 8);
        offset += 10;

        if (type == 33 && rdlen >= 6)
        {
            const unsigned char *rd = answer + offset;
            SRVRecord rec;
            rec.priority = readU16(rd);
            rec.weight = readU16(rd + 2);
            rec.port = readU16(rd + 4);

            int targetOffset = offset + 6;
            expandName(answer, targetOffset, rec.target);

            result.push_back(rec);
        }

        offset += rdlen;
    }

    return result;
}
auto queryDns(std::string n) -> std::vector<OMNetworkDnsResult>
{
    res_init();

    unsigned char answer[65536];
    int len = res_query(("_minecraft._tcp." + n).c_str(), ns_c_in, ns_t_srv, answer, sizeof(answer));
    if (len < 0)
    {
        logger.warn("DNS query failed: {}", hstrerror(h_errno));
        return {};
    }

    auto r = parseSRVResponse(answer, len);
    std::vector<OMNetworkDnsResult> result;
    for (auto &l : r)
    {
        result.push_back({l.target, l.port});
    }
    return result;
}
} // namespace openminecraft::network
