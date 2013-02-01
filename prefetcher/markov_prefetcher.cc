/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"

const unsigned int MISS_MEMORY = 50;
Addr miss_list[MISS_MEMORY];
int last = 0;

const unsigned int MAP_SIZE =16;
unsigned int missed_index[MAP_SIZE];

const unsigned int FETCH_COUNT = 3;

void insert(Addr miss_addr)
{
  miss_list[last++] = miss_addr;
  if (last >= MISS_MEMORY)
  {
    last = 0;
  }
}

void issue_prefetches_from(int miss_index)
{
  for (unsigned int i = miss_index;
       i < miss_index + FETCH_COUNT && i <= last; ++i)
  {
    if (!in_cache(miss_list[i]))
    {
      issue_prefetch(miss_list[i]);
    }
  }
}

void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
}

unsigned int in_miss_list(Addr addr)
{
  for (unsigned int i = 0; i < MISS_MEMORY; ++i)
  {
    if (miss_list[i] == addr)
    {
      return i;
    }
  }
  return -1;
}

void prefetch_access(AccessStat stat)
{
  if (stat.miss)
  {
    int miss_index = in_miss_list(stat.mem_addr);
    if (miss_index != -1) 
    {
      issue_prefetches_from(miss_index);
    }
    else
    {
      insert(stat.mem_addr);
    }
  }
}

void prefetch_complete(Addr addr)
{
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
}
