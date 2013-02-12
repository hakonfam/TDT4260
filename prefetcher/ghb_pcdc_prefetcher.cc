#include "ghb_pcdc.hh"

GHB_PCDC<2048> prefetcher;

void prefetch_init()
{
  
}

void prefetch_access(AccessStat stat)
{
  PrefetchDecision d = prefetcher.react_to_access(stat);
  typedef std::vector<Addr>::const_iterator It;
  for (It it = d.prefetchAddresses.begin(),
           e = d.prefetchAddresses.end(); it != e; ++it) 
  {
    issue_prefetch(*it);
  }
}

void prefetch_complete(Addr addr)
{
  
}
