#include "ghb_pcdc.hh"

GHB_PCDC<256> prefetcher;

void prefetch_init()
{
    DPRINTF(HWPrefetch, "Initializing ghb_pcdc_prefetcher.cc\n");
}

void prefetch_access(AccessStat stat)
{
    PrefetchDecision d = prefetcher.react_to_access(stat);
    DPRINTF(HWPrefetch, "Decided to prefetch %u addresses\n", 
            d.prefetchAddresses.size());

    typedef std::vector<Addr>::const_iterator It;
    for (It it = d.prefetchAddresses.begin(),
             e = d.prefetchAddresses.end(); it != e; ++it) 
    {
        if (!in_cache(*it) && !in_mshr_queue(*it))
        {
            //Should include check for *it <= MAX_PHYS_MEM_ADDR,
            //but keeping it unchecked to get crashes if it happens.
            //NOT
            if (*it <= MAX_PHYS_MEM_ADDR)
            {
                issue_prefetch(*it);
            }
        }
    }
}

void prefetch_complete(Addr addr)
{
  
}
