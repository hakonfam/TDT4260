#include "prefetcher.hh"
#include "interface.hh"

struct GenerationEntry {
  Addr pc;
  uint64_t offset;
  uint64_t tag;
  std::bitset pattern
};
 
struct HistoryEntry {
  // together pc and offset form the tag of the entry
  Addr pc;
  uint64_t offset;
  
  // the recorded pattern
  std::bitset spatial_pattern;
};

public class SMS_Prefetcher : public Prefetcher
{
public:
  virtual ~Prefetcher() {};
  virtual unsigned int prefetch_attempts();
  virtual unsigned int prefetch_hits();
  virtual void increase_aggressiveness();
  virtual void decrease_aggressiveness();
  virtual PrefetchDecision react_to_access(AccessStat stat);
private:
  unsigned int attempts;
  unsigned int hits;

  unsigned int history_table_size;
  unsigned int filter_table_size;
  unsigned int accumulation_table_size;

  vector<GenerationEntry> accumulation_table;
  vector<GenerationEntry> filter_table;

  vector<HistoryEntry> page_history_table;
  
  bool hasEvictions (AccessStat stat);
  bool hasRecordedPattern (Accessstat stat);
  vector<Addr> getRecordedPattern(Accessstat stat);
  bool isTriggerAccess(AccessStat stat);
  void startRecording(AccessStat stat);
  bool isRecordingRegion(AccessStat stat);
  void addToRecording(AccessStat stat);
}
