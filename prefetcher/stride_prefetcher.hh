#ifndef _STRIDE_PREFETCHER_H_
#define _STRIDE_PREFETCHER_H_
#include "stride_stack.hh"	
class StridePrefetcher : public Prefetcher { 
 private:
  unsigned int 	attempts, 
								hits, 
								addressesToPrefetch, 
								hitsBeforePrefetch, 
								numberOfStrides,
								currentDelta;
  bool strideInProgress;
	Addr lastAccessedAddress;
  void updateNumberOfStrides(Addr addr);
	PrefetchDecision getPrefetchedAddresses(Addr address);
 public:
	StridePrefetcher();
  PrefetchDecision react_to_access(AccessStat stat);
	void increase_aggressiveness() {++addressesToPrefetch;};
	void decrease_aggressiveness() {--addressesToPrefetch;};
	unsigned int prefetch_attemps() {return attempts;};
	void prefetch_access();
	void prefetch_init();
	unsigned int prefetch_hits() {return hits;};
};

#endif
