#include <stdint.h>
#include "interface.hh"
#include <bitset>
#include <vector>
#include <cstdio>


using std::vector;

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
		Addr lastPrefetchedAddress;
		void updateNumberOfStrides(Addr addr);
		PrefetchDecision getPrefetchedAddresses(Addr address);
	public:
		StridePrefetcher() {};
		~StridePrefetcher() {};
		PrefetchDecision react_to_access(AccessStat stat);
		void increase_aggressiveness() {++addressesToPrefetch;};
		void decrease_aggressiveness() {--addressesToPrefetch;};
		unsigned int prefetch_attempts() const {return attempts;};
		void prefetch_access();
		void prefetch_init();
		unsigned int prefetch_hits() const {return hits;};
};

StridePrefetcher prefetcher;

void StridePrefetcher::prefetch_init()
{
	hitsBeforePrefetch = 4;
	addressesToPrefetch = 15;
	attempts = 0;
	hits = 0;
	currentDelta = 0;
	lastAccessedAddress = -1;
	numberOfStrides = 0;
}

PrefetchDecision StridePrefetcher::react_to_access(AccessStat stat)
{
	if(!stat.miss)
		return PrefetchDecision();

	Addr addressNumber = stat.mem_addr;


	updateNumberOfStrides(addressNumber);
	if(numberOfStrides >= hitsBeforePrefetch)
		return getPrefetchedAddresses(addressNumber); 
	else
		return PrefetchDecision(); 
}

void StridePrefetcher::updateNumberOfStrides(Addr address)
{
	if(lastAccessedAddress == -1) {
		lastAccessedAddress = address;
		return;
	}
	int diff = address - lastAccessedAddress;
  int diffFromLastPrefetchedAddress = address - lastPrefetchedAddress;
	// The stride is still going
	if(diff == currentDelta || diffFromLastPrefetchedAddress == currentDelta)
	{
		++numberOfStrides;
	}

	// Start a new stride with the new diff
	else
	{
		currentDelta = diff;
		numberOfStrides = 0;
	}

	lastAccessedAddress = address;
}

PrefetchDecision StridePrefetcher::getPrefetchedAddresses(Addr address)
{
	std::vector<Addr> addrs;
	for(int i = 1; i <= addressesToPrefetch; ++i)
	{
		Addr tmp = address + (i * currentDelta);
		addrs.push_back(tmp);  
	}
	lastPrefetchedAddress = address + (addressesToPrefetch * currentDelta);
	return PrefetchDecision(addrs);
}



/*
 * Functions that are called by the simulator, with implementation
 * provided by the user. The implementation may be an empty function.
 */

/*
 * The simulator calls this before any memory access to let the prefetcher
 * initialize itself.
 */
extern "C" void prefetch_init(void)
{
	prefetcher.prefetch_init();
}


/*
 * The simulator calls this function to notify the prefetcher about
 * a cache access (both hits and misses).
 */
extern "C" void prefetch_access(AccessStat stat){ 
	PrefetchDecision d = prefetcher.react_to_access(stat);
	typedef std::vector<Addr>::const_iterator It;
	for (It it = d.prefetchAddresses.begin(),
			e = d.prefetchAddresses.end(); it != e; ++it) 
	{
		if (!in_cache(*it) && !in_mshr_queue(*it))
		{
			if (*it <= MAX_PHYS_MEM_ADDR)
			{
				issue_prefetch(*it);
			}
		}
	}
}

/*
 * The simulator calls this function to notify the prefetcher that
 * a prefetch load to address addr has just completed.
 */
extern "C" void prefetch_complete(Addr addr) {}
