/*#include "resolv.h"
#include <netdb.h>

void test()
{
    res_init();

    unsigned char answer[65536];
    int len = res_query("_minecraft._tcp.awa.kjmc.top", ns_c_in, ns_t_srv, answer, sizeof(answer));
    if (len < 0)
    {
        printf("DNS query failed: %s\n", hstrerror(h_errno));
    }

    ns_msg handle;
    ns_initparse(answer, len, &handle);

    int count = ns_msg_count(handle, ns_s_an);
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

            printf("Priority: %u, Weight: %u, Port: %u, Target: %s\n", priority, weight, port, target);
        }
    }
}*/