#ifndef _PREFETCHER_H_
#define _PREFETCHER_H_

struct PrefetchDecision
{
  explicit PrefetchDecision()
      : wantToPrefetch(false),
        prefetchAddress(0x0) { /*empty */}

  explicit PrefetchDecision(Addr addr)
      : wantToPrefetch(true),
        prefetchAddress(addr) { /* empty */ }

  const bool wantToPrefetch;
  Addr prefetchAddress;
};

class Prefetcher
{
 public:
  virtual ~Prefetcher() {};
  unsigned int prefetch_attempts() const = 0;
  unsigned int prefetch_hits() const = 0;
  void increase_aggressiveness() = 0;
  void decrease_aggressiveness() = 0;
  PrefetchDecision react_to_access(AccessStat stat) = 0;
};

#endif /* _PREFETCHER_H_ */
