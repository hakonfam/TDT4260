#include "stride_stack.hh"
#include "prefetcher.hh"

class StridePrefetcher : public Pretcher
{
 private:
  unsigned int attempts, hits, addressesToPrefetch, hitsBeforePrefetch, nummberOfStrides;
  StrideStack stack;
  bool strideInProgress;
 public:
  void updateNumberOfStrides(int addr);
  PrefetchDecision getPrefetchedAddresses();
};
