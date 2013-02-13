#include "interface.hh"
#include <stdint.h>

/* size of the region in bytes */
const int REGION_SIZE= 2 * 1024;

/* size of the L2 cache in bytes */
const int L2_MEM_SIZE= 256 * 1024 * 1024;

/* number of regions */
const int N_REGIONS = L2_MEM_SIZE / REGION_SIZE;

/* The number of entries in the Active Generation Table
 * In is logically a single table, but is divided in two for convenience
 */
const int N_FILTER_ENTRIES = 32;
const int N_ACC_ENTRIES = 32;

/* numbef of entries in filter table */



SMS_Prefetcher::SMS_Prefetcher() :
  attempts(0), hits(0), history_table_size(64), 
  filter_table_size(32), accumulation_table_size(32)
{
  accumulation_table = new GenerationEntry [accumulation_table_size];
  filter_table = new GenerationEntry [];

  //setup tables
}

SMS_Prefetcher::~SMS_Prefetcher()
{
  
  // destroy tables
}

unsigned int SMS_Prefetcher::prefetch_attempts(){
  return attempts;
}

unsigned int SMS_Prefetcher::prefetch_hits(){
  return hits;
}

void SMS_Prefetcher::increase_aggressiveness() { /* empty */ }
void SMS_Prefetcher::decrease_aggressiveness() { /* empty */ }

PrefetchDecision SMS_Prefetcher:: react_to_access(AccessStat stat){
  // do everything
}

bool SMS_Prefetcher::hasEvictions (AccessStat stat) { /* empty */ }
bool SMS_Prefetcher::hasRecordedPattern (Accessstat stat) { /* empty */ }
vector<Addr> SMS_Prefetcher::getRecordedPattern(Accessstat stat) { /* empty */ }
bool SMS_Prefetcher::isTriggerAccess(AccessStat stat){ /* empty */ }
void SMS_Prefetcher::startRecording(AccessStat stat) { /* empty */ }
bool SMS_Prefetcher::isRecordingRegion(AccessStat stat) { /* empty */ }
void SMS_Prefetcher::addToRecording(AccessStat stat) { /* empty */ }



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
