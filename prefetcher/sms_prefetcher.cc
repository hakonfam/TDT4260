#include <stdint.h>
#include "sms_prefetcher.hh"
#include "prefetcher.hh"
#include <vector>

using std::vector;

static SMS_Prefetcher prefetcher;
/* numbef of entries in filter table */


static inline uint64_t getOffset(Addr addr) {return (addr & (( 1<<REGION_SIZE_LOG_2 ) - 1)) / BLOCK_SIZE;}
static inline uint64_t getRegion(Addr addr) {return addr & ~(( 1<<REGION_SIZE_LOG_2) - 1);}

static void insertGeneration(GenerationEntry entry, vector<GenerationEntry> &table, int index, int table_size){
    for(int i = 0; i < table_size; i++){
	int candidateIndex = (index + i) % table_size;
	GenerationEntry element = table.at(candidateIndex);
	if(entry.pc == 0 && entry.offset == 0) // assume this is not in use
	{
	    table.at(candidateIndex) = entry; 
	    return;
	}
    }
    table.at(index) = entry;
} 

static int findGeneration(AccessStat stat, vector<GenerationEntry> &table, int table_size){
    int index = 0;
    for(vector<GenerationEntry>::const_iterator it = table.begin(); it != table.end(); it++, index++){
	if(it->pc == stat.pc && it->offset == getOffset(stat.mem_addr)){
	    return index;
	}
    }
    return -1;
}

SMS_Prefetcher::SMS_Prefetcher() :
  attempts(0), hits(0), 
  history_table_size(64), 
  filter_table_size(32), accumulation_table_size(32),
  filter_table_index(0), accumulation_table_index(0)
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
	if(hasRecordedPattern(stat)){
	    addr = getRecordedPattern(stat);
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

bool SMS_Prefetcher::hasEvictions (AccessStat stat) { /* empty */ }
bool SMS_Prefetcher::hasRecordedPattern (AccessStat stat) { /* empty */ }
vector<Addr> SMS_Prefetcher::getRecordedPattern(AccessStat stat) { /* empty */ }

bool SMS_Prefetcher::isTriggerAccess(AccessStat stat){ 
    int index = findGeneration(stat, filter_table, filter_table_size);
    if (index != -1) return false;
    index = findGeneration(stat, accumulation_table, accumulation_table_size);
    if( index != -1) return false;
    return true;
}

void SMS_Prefetcher::startRecording(AccessStat stat) {
    GenerationEntry entry(stat.pc,getOffset(stat.mem_addr), getRegion(stat.mem_addr));
    insertGeneration(entry, filter_table, filter_table_index, filter_table_size);
    filter_table_index = (filter_table_index + 1) % filter_table_size;
}

void SMS_Prefetcher::stopRecording(AccessStat stat) {
    int index = findGeneration(stat, filter_table, filter_table_size);
    if (index != -1){
	GenerationEntry emptyEntry;
	filter_table.at(index) = emptyEntry;
	return;
    }
    index = findGeneration(stat, accumulation_table, accumulation_table_size);
    if ( index != -1){
	
// move to history table
	return;
    }
    
    
}

void SMS_Prefetcher::addToRecording(AccessStat stat) { 
    int index = findGeneration(stat, filter_table, filter_table_size);
    if (index != 0){
    }
    index = findGeneration( stat, accumulation_table, accumulation_table_size);
    if(index != 0){
    }
    
    for(vector<GenerationEntry>::const_interator it = accumulation_table.begin(); it != accumulation_table.end(); it++){
	if(it->pc == stat.pc && it->offset == getOffset(stat.mem_addr)){
	    // update bit pattern

	    return;
	    
	}
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


  /* if is trigger action
     else recording
   */
  // check if recorded pattern exists,
  // if pattern exists fetch all addresses in region

  // find region of access
  // if page in region has been evicted stop recording and add to history table
  
  // if trigger access start recording in region
  
  // if access to a recording region, add to recording

  
}

/*
 * The simulator calls this function to notify the prefetcher that
 * a prefetch load to address addr has just completed.
 */
extern "C" void prefetch_complete(Addr addr){ /* empty */ }
