#include <stdint.h>
#include "interface.hh"
#include <bitset>
#include <vector>
#include <cstdio>

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

struct Node{
    uint64_t timestamp, addr;

    Node(uint64_t ts, uint64_t a) : timestamp(ts), addr(a){}

    bool operator<(const Node &other) {
        return timestamp < other.timestamp;
    }
};

const unsigned int MAX_NODE_COUNT = 6000;

struct Transition
{
  int node;
  unsigned int uses;
  Transition(int n, unsigned u) : node(n), uses(u) { /*empty */}
};

class MarkovPrefetcher : public Prefetcher {
    private:
        unsigned int attempts,
                     hits;
        const unsigned int LOOKAHEAD_LIMIT;
        std::vector<Node> nodes;
        int last;
  std::vector<std::vector<Transition> > transitions;
    public:
  MarkovPrefetcher() : LOOKAHEAD_LIMIT(4), last(-1), transitions(MAX_NODE_COUNT) {}
        PrefetchDecision react_to_access(AccessStat stat);
        void increase_aggressiveness(){};
        void decrease_aggressiveness(){};
        unsigned int prefetch_attempts() const {return attempts;};
        void prefetch_access(void);
        void prefetch_init(void);
        unsigned int prefetch_hits() const {return hits;};
        int find_node(AccessStat stat);
        int most_probable_transition_from(int n) const;
};

MarkovPrefetcher prefetcher;

void MarkovPrefetcher::prefetch_init()
{
}

/* Prefetches based on generated markov map, lookaheadLimit steps ahead */
PrefetchDecision MarkovPrefetcher::react_to_access(AccessStat stat)
{
  if (!stat.miss)
  {
    return PrefetchDecision();
  }

    /* Find/Create node in map */ 
    int node = find_node(stat);

    if (last != -1)
    {
      std::vector<Transition>::iterator it, e;
      for (it = transitions[last].begin(),
               e = transitions[last].end(); it != e; ++it) 
      {
        if (it->node == node)
        {
          break;
        }
      }
      if (it != e)
      {
        it->uses++;
      }
      else
      {
        transitions[last].push_back(Transition(node, 1));
      }
    }

    last = node;

    std::vector<Addr> prefetch_addresses;

    for(int nextNode = most_probable_transition_from(node);
        nextNode != -1 && prefetch_addresses.size() < LOOKAHEAD_LIMIT;
        nextNode = most_probable_transition_from(nextNode)) {
      prefetch_addresses.push_back(nodes[nextNode].addr);
    }

    return PrefetchDecision(prefetch_addresses);
}

int MarkovPrefetcher::find_node(AccessStat stat)
{
  int oldest = 0;
  uint64_t oldest_ts = stat.time;
  for (int i = 0; i < nodes.size(); ++i)
  {
    if (nodes[i].addr == stat.mem_addr)
    {
      nodes[i].timestamp = stat.time;
      return i;
    }
    else
    {
      if (nodes[i].timestamp < oldest_ts)
      {
        oldest = i;
        oldest_ts = nodes[i].timestamp;
      }
    }
  }
  if (nodes.size() == MAX_NODE_COUNT)
  {
    nodes[oldest] = Node(stat.time, stat.mem_addr);
    transitions[oldest].clear();
    for (int i = 0; i < MAX_NODE_COUNT; ++i)
    {
      typedef std::vector<Transition>::iterator It;
      for (It it = transitions[i].begin(),
               e = transitions[i].end(); it != e; ++it)
      {
        if (it->node == oldest)
        {
          transitions[i].erase(it);
          break;
        }
      }
    }
    return oldest;
  }
  else
  {
    nodes.push_back(Node(stat.time, stat.mem_addr));
    return nodes.size() - 1;
  }
}

int MarkovPrefetcher::most_probable_transition_from(int n) const
{
  Transition mostProbable(-1, 0);
  for (int i = 0; i < transitions[n].size(); ++i)
  {
    if (transitions[n][i].uses > mostProbable.uses)
    {
      mostProbable = transitions[n][i];
    }
  }
  return mostProbable.node;
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
    /* Locate node to prefetch */
    PrefetchDecision d = prefetcher.react_to_access(stat);
    printf( "Decided to prefetch %lu addresses\n", d.prefetchAddresses.size());

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
extern "C" void prefetch_complete(Addr addr){ /* empty */ }
