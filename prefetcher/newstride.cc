#include <vector>
#include "interface.hh"

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

class StrideStack
{
 private:
  	unsigned int currentSize, index;
  	unsigned int stack[5];
 public:
  	void push(Addr value);
	void printStack();
	unsigned int getCurrentSize();
	unsigned int getItemAt(int index);
};

class StridePrefetcher { 
 private:
  unsigned int 	attempts, 
								hits, 
								addressesToPrefetch, 
								hitsBeforePrefetch, 
								numberOfStrides,
								currentDelta;
  StrideStack* stack;
  bool strideInProgress;
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

void StrideStack::push(Addr value)
{
 for(int i = 4; i > 0; --i)
  this->stack[i] = this->stack[i-1];
 
 if(currentSize < 5) currentSize++;

 this->stack[0] = value;
}

unsigned int StrideStack::getCurrentSize() 
{
 return this->currentSize;
}

unsigned int StrideStack::getItemAt(int index)
{
 return this->stack[index];
}

StridePrefetcher sp;

void prefetch_init(void)
{
	sp.prefetch_init();
}

void prefetch_access(AccessStat stat)
{
    PrefetchDecision d = sp.react_to_access(stat);

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
