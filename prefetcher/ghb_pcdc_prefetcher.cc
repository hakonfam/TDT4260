#include "ghb_pcdc.hh"

GHB_PCDC<256> prefetcher;

void prefetch_init()
{
  
}

void prefetch_access(AccessStat stat)
{
    PrefetchDecision d = prefetcher.react_to_access(stat);
    DPRINTF(HWPrefetch, "Decided to prefetch %u addresses\n", d.size());

    typedef std::vector<Addr>::const_iterator It;
    for (It it = d.prefetchAddresses.begin(),
             e = d.prefetchAddresses.end(); it != e; ++it) 
    {
        if (!in_cache(*it) && !in_mshr_queue(*it))
        {
            issue_prefetch(*it);
        }
    }
}

void prefetch_complete(Addr addr)
{
  
}
