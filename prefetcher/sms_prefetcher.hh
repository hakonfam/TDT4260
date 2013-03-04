#ifndef _SMS_PREFETCHER_HH_
#define _SMS_PREFETCHER_HH_
#include "prefetcher.hh"
#include <bitset>
#include <vector>

const int REGION_SIZE_LOG_2 = 11;
const int REGION_SIZE = 1 << REGION_SIZE_LOG_2;
const int N_BITS = REGION_SIZE / BLOCK_SIZE;

struct GenerationEntry {
    Addr pc;
    uint64_t offset;
    uint64_t tag;
    std::bitset<N_BITS> pattern;

    GenerationEntry(Addr pc = 0, uint64_t offset = 0, uint64_t tag = 0):
      pc(pc), offset(offset), tag(tag), pattern()
  { /* empty */ }
};
 
struct HistoryEntry {
  // together pc and offset form the tag of the entry
  Addr pc;
  uint64_t offset;
  
  // the recorded pattern
  std::bitset<N_BITS> spatial_pattern;

  HistoryEntry(Addr pc = 0, uint64_t offset = 0 ):
    pc(pc), offset(offset),
    spatial_pattern()
    { /* empty */ }
};

class SMS_Prefetcher : public Prefetcher
{
public:
  SMS_Prefetcher();
  virtual ~SMS_Prefetcher() ;
  virtual unsigned int prefetch_attempts() const;
  virtual unsigned int prefetch_hits() const;
  virtual void increase_aggressiveness();
  virtual void decrease_aggressiveness();
  virtual PrefetchDecision react_to_access(AccessStat stat);
private:
  unsigned int attempts;
  unsigned int hits;

  unsigned int history_table_size;
  unsigned int filter_table_size;
  unsigned int accumulation_table_size;

    unsigned int filter_table_index;
    unsigned int accumulation_table_index;

  /* together these tables make up the active generation table */
    std::vector<GenerationEntry>  accumulation_table;
    std::vector<GenerationEntry>  filter_table;

    std::vector<HistoryEntry> page_history_table;
  
  bool hasEvictions (AccessStat stat);
  bool hasRecordedPattern (AccessStat stat);
    std::vector<Addr> getRecordedPattern(AccessStat stat);
  bool isTriggerAccess(AccessStat stat);
  void startRecording(AccessStat stat);
  void stopRecording(AccessStat stat);
  void addToRecording(AccessStat stat);
};
#endif
