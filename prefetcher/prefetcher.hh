#ifndef _PREFETCHER_H_
#define _PREFETCHER_H_

#include <vector>

struct PrefetchDecision
{
  explicit PrefetchDecision()
      : wantToPrefetch(false),
        prefetchAddresses() { /*empty */}

  explicit PrefetchDecision(const std::vector<Addr> &addrs)
      : wantToPrefetch(true),
        prefetchAddresses(addr) { /* empty */ }

  const bool wantToPrefetch;
  std::vector<Addr> prefetchAddresses_;
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
