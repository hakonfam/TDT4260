#ifndef _PREFETCHER_H_
#define _PREFETCHER_H_

#include "interface.hh"

#include <vector>

struct PrefetchDecision
{
  explicit PrefetchDecision()
      : prefetchAddresses() { /*empty */}

  explicit PrefetchDecision(const std::vector<Addr> &addrs)
      : prefetchAddresses(addrs) { /* empty */ }

  std::vector<Addr> prefetchAddresses;
};

class Prefetcher
{
 public:
  virtual ~Prefetcher() {};
  virtual unsigned int prefetch_attempts() const = 0;
  virtual unsigned int prefetch_hits() const = 0;
  virtual void increase_aggressiveness() = 0;
  virtual void decrease_aggressiveness() = 0;
  virtual PrefetchDecision react_to_access(AccessStat stat) = 0;
};

#endif /* _PREFETCHER_H_ */
