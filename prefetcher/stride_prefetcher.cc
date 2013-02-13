#include "stride_prefetcher.hh"

StridePrefetcher::StridePrefetcher()
{
 attempts = 0;
 hits = 0;
 currentDelta = 0;
}

PrefetchDecision StridePrefetcher::react_to_access(AccessStat stat)
{
 unsigned int addr = stat.mem_addr;
 stack->push(addr);

 updateNumberOfStrides(addr);
 if(numberOfstrides >= hitsBeforePrefetch)
  return getPrefetchedAddresses(); 
 else
  return new PrefetchDecision(); 
}

void StridePrefetcher::updateNumberOfStrides(int mem_addr)
{
 if(stack->getCurrentSize() < 2) return;
 int diff = stack->getValueAt(0) - stack->getValueAt(1);

 if(diff == currentDelta)
  ++numberOfStrides;
  
  else
  {
   currentDelta = diff;
   numberOfStrides = 1;
  }
}

PrefetchDecision StridePrefetcher::getPrefetchedAddresses()
{
 for(int i = 0; i < addressesToPrefetch; ++i)
 {
  
 } 
}


