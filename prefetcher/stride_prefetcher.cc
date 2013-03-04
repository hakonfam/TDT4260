#include "stride_prefetcher.hh"
#include "stride_stack.hh"
#include <vector>

StridePrefetcher::StridePrefetcher()
{
	attempts = 0;
	hits = 0;
	currentDelta = 0;
}

PrefetchDecision StridePrefetcher::react_to_access(AccessStat stat)
{
 Addr addr = stat.mem_addr;
 stack->push(addr);

 updateNumberOfStrides(addr);
 if(numberOfStrides >= hitsBeforePrefetch)
  return getPrefetchedAddresses(addr); 
 else
  return PrefetchDecision(); 
}

void StridePrefetcher::updateNumberOfStrides(Addr mem_addr)
{
 if(stack->getCurrentSize() < 2) return;
 int diff = stack->getItemAt(0) - stack->getItemAt(1);

 if(diff == currentDelta)
  ++numberOfStrides;
  
  else
  {
   currentDelta = diff;
   numberOfStrides = 1;
  }
}

PrefetchDecision StridePrefetcher::getPrefetchedAddresses(Addr address)
{
	std::vector<Addr> addrs;
 	for(int i = 1; i <= addressesToPrefetch; ++i)
 	{
		addrs.push_back(address + (i*currentDelta));  
 	}
	return PrefetchDecision(addrs);
}
