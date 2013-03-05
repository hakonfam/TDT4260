#include <stdint.h>
#include "interface.hh"
#include <bitset>
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


const int REGION_SIZE_LOG_2 = 11;
const int REGION_SIZE = 1 << REGION_SIZE_LOG_2;
const long unsigned int N_BITS = REGION_SIZE / BLOCK_SIZE;

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
    unsigned int history_table_index;
  /* together these tables make up the active generation table */
    std::vector<GenerationEntry>  accumulation_table;
    std::vector<GenerationEntry>  filter_table;

    std::vector<HistoryEntry> page_history_table;
  
  bool hasEvictions (AccessStat stat);
    int findRecordedPattern (AccessStat stat);
    std::vector<Addr> getRecordedPattern(AccessStat stat, int index);
  bool isTriggerAccess(AccessStat stat);
  void startRecording(AccessStat stat);
  void stopRecording(AccessStat stat);
  void addToRecording(AccessStat stat);
};

using std::vector;

static SMS_Prefetcher prefetcher;
/* number of entries in filter table */


static inline uint64_t getOffset(Addr addr) {return (addr & (( 1<<REGION_SIZE_LOG_2 ) - 1)) / BLOCK_SIZE;}
static inline uint64_t getRegion(Addr addr) {return addr & ~(( 1<<REGION_SIZE_LOG_2) - 1);}


static void insertGeneration(GenerationEntry entry, vector<GenerationEntry> &table, int index, int table_size){
    for(int i = 0; i < table_size; i++){
	int candidateIndex = (index + i) % table_size;
	GenerationEntry element = table.at(candidateIndex);
	if(element.pc == 0 && element.offset == 0) // assume this is not in use
	{
	    table.at(candidateIndex) = entry; 
	    return;
	}
    }
    table.at(index) = entry;
} 

static int findGeneration(AccessStat stat, vector<GenerationEntry> &table){
    int index = 0;
    for(vector<GenerationEntry>::const_iterator it = table.begin(); it != table.end(); it++, index++){
	if(it->pc == stat.pc && it->offset == getOffset(stat.mem_addr) && it->tag == getRegion(stat.mem_addr)){
	    return index;
	}
    }
    return -1;
}


SMS_Prefetcher::SMS_Prefetcher() :
  attempts(0), hits(0), 
  history_table_size(64), 
  filter_table_size(32), accumulation_table_size(32),
  filter_table_index(0), accumulation_table_index(0),
    history_table_index(0),
  accumulation_table(),
  filter_table(),
  page_history_table()
{
    accumulation_table = vector<GenerationEntry>(accumulation_table_size);
    accumulation_table.reserve(accumulation_table_size);

    filter_table = vector<GenerationEntry>(filter_table_size);
    filter_table.reserve(filter_table_size);

    page_history_table = vector<HistoryEntry>(history_table_size);
    page_history_table.reserve(history_table_size);
}

SMS_Prefetcher::~SMS_Prefetcher()
{
  
}

unsigned int SMS_Prefetcher::prefetch_attempts() const {
  return attempts;
}

unsigned int SMS_Prefetcher::prefetch_hits() const{
  return hits;
}

void SMS_Prefetcher::increase_aggressiveness() { /* empty */ }
void SMS_Prefetcher::decrease_aggressiveness() { /* empty */ }

PrefetchDecision SMS_Prefetcher:: react_to_access(AccessStat stat){
    vector<Addr> addr;
    if(isTriggerAccess(stat)){
	int index = findRecordedPattern(stat);
	if( index != -1){
	    addr = getRecordedPattern(stat, index);
	}
	startRecording(stat);
    } else {
	// recording 
	if( hasEvictions(stat)){
	    stopRecording(stat);
	}
	else {
	    addToRecording(stat);
	}
	  
    }    
    return PrefetchDecision(addr);
}

bool SMS_Prefetcher::hasEvictions (AccessStat stat) { 
    int index = findGeneration(stat, filter_table);
    if(index != -1) {
	GenerationEntry entry = filter_table.at(index);
	Addr memAddr = entry.tag | (entry.offset);
	return !in_cache(memAddr);
    } 

    index = findGeneration(stat, accumulation_table);
    if(index != -1){
	GenerationEntry entry = accumulation_table.at(index);
	Addr baseAddr = stat.mem_addr;
	for(int i = 0; i < entry.pattern.size() ; i++){
	    if(entry.pattern.test(i)){
		Addr memAddr = baseAddr | (i * BLOCK_SIZE);
		if(!in_cache(memAddr)){
		    return true;
		}
	    }
	}
    }
    return false;
}


int SMS_Prefetcher::findRecordedPattern (AccessStat stat) {
    int index = 0;
    for(vector<HistoryEntry>::const_iterator it = page_history_table.begin(); it != page_history_table.end(); it++, index++){
	if(it->pc == stat.pc && it->offset == stat.mem_addr){
	    return index;
	}
    }
    return -1;
    
}

vector<Addr> SMS_Prefetcher::getRecordedPattern(AccessStat stat, int index) { 
    HistoryEntry entry = page_history_table.at(index);
    vector<Addr> addresses;
    Addr baseAddr = getRegion(stat.mem_addr);
    for(int i = 0; i < entry.spatial_pattern.size(); i++){
	Addr memAddr = baseAddr | (i * BLOCK_SIZE);
	addresses.push_back(memAddr);
    }
    return addresses;

}

bool SMS_Prefetcher::isTriggerAccess(AccessStat stat){ 
    int index = findGeneration(stat, filter_table);
    if (index != -1) return false;
    index = findGeneration(stat, accumulation_table);
    if( index != -1) return false;
    return true;
}

void SMS_Prefetcher::startRecording(AccessStat stat) {
    GenerationEntry entry(stat.pc,getOffset(stat.mem_addr), getRegion(stat.mem_addr));
    insertGeneration(entry, filter_table, filter_table_index, filter_table_size);
    filter_table_index = (filter_table_index + 1) % filter_table_size;
}

void SMS_Prefetcher::stopRecording(AccessStat stat) {
    int index = findGeneration(stat, filter_table );
    if (index != -1){
	GenerationEntry emptyEntry;
	filter_table.at(index) = emptyEntry;
	return;
    }
    index = findGeneration(stat, accumulation_table);
    if ( index != -1){
	GenerationEntry entry = accumulation_table.at(index);
	HistoryEntry hentry(stat.pc, getOffset(stat.mem_addr));
	hentry.spatial_pattern = entry.pattern;

	for(int i = 0; i < history_table_size; i++){
	    int candidateIndex = (history_table_index + i ) % history_table_size;
	    HistoryEntry element = page_history_table.at(candidateIndex);
	    if(element.pc == 0 && element.offset == 0){
		page_history_table.at(candidateIndex) = hentry;
		history_table_index = (history_table_index + 1) % history_table_size; 
		return;
	    }
	}
	page_history_table.at(history_table_index) = hentry;
	history_table_index = (history_table_index + 1) % history_table_size; 
    }
}

void SMS_Prefetcher::addToRecording(AccessStat stat) { 
    int index = findGeneration(stat, filter_table);
    if (index != -1){
	GenerationEntry entry = filter_table.at(index);
	if( entry.offset != getOffset(stat.mem_addr)){
	    // second access within region, move to accumulation table
	    filter_table.at(index) = GenerationEntry();
	    entry.pattern |= (1 << entry.offset);
	    entry.pattern |= (1 << getOffset(stat.mem_addr));
	    insertGeneration(entry, accumulation_table, accumulation_table_index, accumulation_table_size);
	}
	return;

    }
    index = findGeneration( stat, accumulation_table);
    if(index != -1){
	// update bit pattern
	accumulation_table.at(index).pattern |= (1 << getOffset(stat.mem_addr)); 
    }
    
}




/*
 * Functions that are called by the simulator, with implementation
 * provided by the user. The implementation may be an empty function.
 */

/*
 * The simulator calls this before any memory access to let the prefetcher
 * initialize itself.
 */
extern "C" void prefetch_init(void){ /* empty */ }

/*
 * The simulator calls this function to notify the prefetcher about
 * a cache access (both hits and misses).
 */
extern "C" void prefetch_access(AccessStat stat){ 

  

  
}

/*
 * The simulator calls this function to notify the prefetcher that
 * a prefetch load to address addr has just completed.
 */
extern "C" void prefetch_complete(Addr addr){ /* empty */ }
