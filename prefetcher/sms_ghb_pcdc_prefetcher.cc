#include <stdint.h>
#include "interface.hh"
#include <bitset>
#include <vector>
#include <cstdio>


#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <set>




/* PREFETCHER INTERFACE */

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


/* GHB PCDC PREFETCHER */

struct TableEntry
{
    explicit TableEntry() : address(0x0), previousMiss(0) 
        { 
            //DPRINTF(HWPrefetch, "Creating new TableEntry()\n");
        }
    explicit TableEntry(Addr addr, TableEntry *prevMiss)
        : address(addr), previousMiss(prevMiss) 
        {
            //DPRINTF(HWPrefetch, 
//                    "Creating new TableEntry(addr, prev)\n");
        }
    ~TableEntry()
        {
            //DPRINTF(HWPrefetch, "Destroyhing a TableEntry\n");            
        }
    Addr address;
    //Pointer member, does this mean we need copy ctor, assignment op
    //and destructor? In this case, it's a pointer we do not own so
    //we should not delete it when the TableEntry is deleted, and when
    //copying a TableEntry it makes sense to copy the pointer instead 
    //of its value - right..?
    TableEntry *previousMiss;
// private:
//     //Disallow copying..?
//     TableEntry& operator=(const TableEntry&);
//     TableEntry(const TableEntry&);

};

template <unsigned int TableSize>
class GlobalHistoryBuffer
{
public:
    explicit GlobalHistoryBuffer()
        : head_(0), evictingOldEntry_(false) 
        { 
            //DPRINTF(HWPrefetch, "Creating new GHB\n");
            memset (buffer_, 0, sizeof(buffer_));
        }
    TableEntry* insert(const AccessStat &stat, TableEntry *previousMiss);
private:
    TableEntry* findFirstEntryReferencing(const TableEntry * const e);
    std::size_t head_;
    bool evictingOldEntry_;
    TableEntry buffer_[TableSize];
    //Disallow copying
    GlobalHistoryBuffer& operator=(const GlobalHistoryBuffer&);
    GlobalHistoryBuffer(const GlobalHistoryBuffer&);
};

template<unsigned int TableSize>
TableEntry* GlobalHistoryBuffer<TableSize>::findFirstEntryReferencing(const TableEntry *const e)
{
    TableEntry *first = 0;
    unsigned int num = 0;
    for (int i = head_; i >= 0; i--)
    {
        if (buffer_[i].previousMiss == e)
        {
            if (num == 0)
            {
                first = &buffer_[i];
            }
            num++;
        }
    }
    for (int i = TableSize - 1; i > head_; i--)
    {
        if (buffer_[i].previousMiss == e)
        {
            if (num == 0)
            {
                first =  &buffer_[i];
            }
            num++;
        }
    }
    assert(num <= 1 && 
           "Should not have more access to same previous miss.");
    return first;
}


template <unsigned int TableSize>
TableEntry* GlobalHistoryBuffer<TableSize>::insert(
    const AccessStat &stat,
    TableEntry *previousMiss)
{
    if (evictingOldEntry_)
    {
        assert(buffer_[head_].previousMiss == 0 &&
               "Evicted entry is first in the queue, and "
               "should not point to any elements in front");
        TableEntry *e = findFirstEntryReferencing(&buffer_[head_]);
        if (e != 0)
        {
            //DPRINTF(HWPrefetch,
//                    "Kicking out referenced element, unlinking it\n");
            e->previousMiss = 0;
        }
        else
        {
            //DPRINTF(HWPrefetch, "Element was not referenced\n");
        }
        //If we are kicking out the previous miss, do not link to it...
        if (&buffer_[head_] == previousMiss)
        {
            previousMiss = 0;
        }
    }
//    buffer_[head_] = TableEntry(stat.mem_addr, previousMiss);
//    TableEntry *newEntry = &buffer_[head_];
    TableEntry *newEntry =
        new (&buffer_[head_]) TableEntry(stat.mem_addr, previousMiss);
    head_++;
    if (head_ >= TableSize)
    {
        head_ = 0;
        evictingOldEntry_ = true;
    }
    return newEntry;
}

template <unsigned int TableSize>
class IndexTable
{
public:
    explicit IndexTable()
        : head_(0)
        {
            //DPRINTF(HWPrefetch, "Creating new Index Table\n");
            memset(buffer_, 0, sizeof(buffer_));
        }

    //Overload [] instead? :x
    TableEntry* previousAccessTo(Addr pc) const;
    void setPreviousAccessTo(Addr pc, TableEntry *e);
private:
    TableEntry buffer_[TableSize];
    std::size_t head_;
    //Disallow copying
    IndexTable& operator=(const IndexTable&);
    IndexTable(const IndexTable&);
};

template<unsigned int TableSize>
TableEntry* IndexTable<TableSize>::previousAccessTo(Addr pc) const
{
    std::size_t index = pc % TableSize;
    if (buffer_[index].address == pc)
    {
        return buffer_[index].previousMiss;
    }
    return 0;
}

template<unsigned int TableSize>
void IndexTable<TableSize>::setPreviousAccessTo(Addr pc, TableEntry *e)
{
    //Is this correct..?
    //Idea: If e is the address of a miss which an index table entry
    //has recorded as the address of its previous miss, then that miss
    //must have been overwritten. Thus, it should be cleared.
    for (int i = 0; i < TableSize; ++i)
    {
        if (buffer_[i].previousMiss == e)
        {
            buffer_[i].previousMiss = 0;
        }
    }
    std::size_t index = pc % TableSize;
    new (&buffer_[index]) TableEntry(pc, e);
//    buffer_[index] = TableEntry(pc, e);
}


template <unsigned int TableSize>
class GHB_PCDC : public Prefetcher
{
public:
    explicit GHB_PCDC()
        : attempts_(0),
          hits_(0),
          numBlocksToPrefetch_(2),
          historyBuffer_(),
          indexTable_() 
        { 
            //DPRINTF(HWPrefetch, "Creating GHB_PCDC\n");
        }
    unsigned int prefetch_attempts() const { return attempts_; }
    unsigned int prefetch_hits() const { return hits_; }
    void increase_aggressiveness() {
        if (numBlocksToPrefetch_ > 1)
            numBlocksToPrefetch_--;
        
        //DPRINTF(HWPrefetch, "Decreased aggressiveness to %u", 
//                numBlocksToPrefetch_);
    }
    void decrease_aggressiveness() {
        numBlocksToPrefetch_++;
        //DPRINTF(HWPrefetch, "Increased aggressiveness to %u", 
//                numBlocksToPrefetch_);
    }
    PrefetchDecision react_to_access(AccessStat stat);
private:
    void insert(const AccessStat &stat);
  
    std::vector<int> compute_delta_table(
        const AccessStat &stat) const;

    int pastPreviousOccurrenceOfLastPair(const std::vector<int> &deltas) const;

    unsigned int attempts_;
    unsigned int hits_;
    unsigned int numBlocksToPrefetch_;
    GlobalHistoryBuffer<TableSize> historyBuffer_;
    IndexTable<TableSize> indexTable_;
};

template<unsigned int TableSize>
void GHB_PCDC<TableSize>::insert(const AccessStat &stat)
{
    TableEntry *lastEntry = indexTable_.previousAccessTo(stat.pc);
    TableEntry *newEntry = historyBuffer_.insert(stat, lastEntry);
    indexTable_.setPreviousAccessTo(stat.pc, newEntry);
}


template<unsigned int TableSize>
std::vector<int> GHB_PCDC<TableSize>::compute_delta_table(
    const AccessStat &stat) const
{
    std::vector<int> deltaTable(0);
    std::vector<TableEntry*> seen;
    for (TableEntry *e = indexTable_.previousAccessTo(stat.pc);
         e != 0 && e->previousMiss != 0; e = e->previousMiss)
    {
        typedef std::vector<TableEntry*>::const_iterator It;
        It prevOccurrence = find(seen.begin(), seen.end(), e);
        if (prevOccurrence != seen.end())
        {
            //DPRINTF(HWPrefetch, "Error, detected cycle in GHB\n");
            //DPRINTF(HWPrefetch, "Cycle: %#x", *prevOccurrence);
            ++prevOccurrence;
            for (; prevOccurrence != seen.end(); ++prevOccurrence)
            {
                //DPRINTF(HWPrefetch, ", %#x", *prevOccurrence);
            }
            //DPRINTF(HWPrefetch, ", %#x", e);
            abort();
        }
        seen.push_back(e);
        deltaTable.push_back(e->address - e->previousMiss->address);
        //DPRINTF(HWPrefetch, "Added another delta\n");
    }
    return deltaTable;
}

template<unsigned int TableSize>
int GHB_PCDC<TableSize>::pastPreviousOccurrenceOfLastPair(
    const std::vector<int> &deltas) const
{
    if (deltas.size() < 4) return -1;

    int d1 = deltas.at(deltas.size() - 2),
        d2 = deltas.at(deltas.size() - 1);
    for (int i = deltas.size() - 4; i >= 0; i--)
    {
        if (deltas.at(i) == d1 && deltas.at(i+1) == d2)
            return i + 2;
    }
    return -1;
}

template<unsigned int TableSize>
PrefetchDecision GHB_PCDC<TableSize>::react_to_access(AccessStat stat)
{
    //ONLY DO THIS ON A MISS! But debug cycle first...
    if (!stat.miss) 
    {
        return PrefetchDecision();
    }
    insert(stat);
    std::vector<int> deltas = compute_delta_table(stat);
    //DPRINTF(HWPrefetch, "Size of delta table: %u\n", deltas.size());
    int index = pastPreviousOccurrenceOfLastPair(deltas);
    if (index == -1)
    {
        return PrefetchDecision(); //Do not prefetch
    }
    else
    {
        std::vector<Addr> addrs;
        std::size_t endFetchIndex = 
            static_cast<std::size_t>(index + numBlocksToPrefetch_);
        Addr prevAddr = stat.mem_addr;
        for (int i = index,
                 e = std::min(endFetchIndex, deltas.size());
             i < e; ++i)
        {
            addrs.push_back(prevAddr + deltas.at(i));
            prevAddr += deltas.at(i);
        }
        return PrefetchDecision(addrs);
    }
}





/* SMS PREFETCHER */

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




static inline uint64_t getOffset(Addr addr) {return (addr & (( 1<<REGION_SIZE_LOG_2 ) - 1)) / BLOCK_SIZE;}
static inline uint64_t getRegion(Addr addr) {return addr & ~(( 1<<REGION_SIZE_LOG_2) - 1);}


static void insertGeneration(GenerationEntry entry, vector<GenerationEntry> &table, int index, int table_size){
    for(int i = 0; i < table_size; i++){
	int candidateIndex = (index + i) % table_size;
	GenerationEntry element = table.at(candidateIndex);
	if(element.pc == 0 && element.offset == 0) // assume this is not in use
	{
	    //printf( "Inserted at index %d\n", candidateIndex);
	    table.at(candidateIndex) = entry; 
	    return;
	}
    }
    //printf( "Inserted at index %d\n", index);
    table.at(index) = entry;
} 

static int findGeneration(AccessStat stat, vector<GenerationEntry> &table){
    int index = 0;
    for(vector<GenerationEntry>::const_iterator it = table.begin(); it != table.end(); it++, index++){
	if(it->tag == getRegion(stat.mem_addr)){
	    return index;
	}
    }
    return -1;
}


SMS_Prefetcher::SMS_Prefetcher() :
  attempts(0), hits(0), 
  history_table_size(256), 
  filter_table_size(32), accumulation_table_size(64),
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
	  //printf( "Found recorded pattern at index %d in page history table PC: %lu TAG: %lu OFFSET: %lu\n",index, stat.pc, getRegion(stat.mem_addr), getOffset(stat.mem_addr));
	    addr = getRecordedPattern(stat, index);
	}
	//printf( "Starting to Record PC: %lu TAG: %lu OFFSET: %lu\n", stat.pc, getRegion(stat.mem_addr), getOffset(stat.mem_addr));
	startRecording(stat);
    } else {
	// recording 
	if( hasEvictions(stat)){
	    //printf( "Found evictions, stopping to record PC: %lu TAG: %lu OFFSET: %lu\n", stat.pc, getRegion(stat.mem_addr), getOffset(stat.mem_addr));
	    stopRecording(stat);
	}
	else {
	  //printf("Adding to recording, PC: %lu TAG: %lu OFFSET: %lu\n", stat.pc, getRegion(stat.mem_addr), getOffset(stat.mem_addr));
	    addToRecording(stat);
	}
	  
    }    
    return PrefetchDecision(addr);
}

bool SMS_Prefetcher::hasEvictions (AccessStat stat) { 
    int index = findGeneration(stat, filter_table);
    if(index != -1) {
	GenerationEntry entry = filter_table.at(index);
	Addr memAddr = entry.tag | (entry.offset * BLOCK_SIZE);
	return !in_cache(memAddr);
    } 

    index = findGeneration(stat, accumulation_table);
    if(index != -1){
	GenerationEntry entry = accumulation_table.at(index);
	Addr baseAddr = entry.tag;
	for(size_t i = 0; i < entry.pattern.size() ; i++){
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
      if(it->pc == stat.pc && it->offset == getOffset(stat.mem_addr)){
	    return index;
	}
    }
    return -1;
    
}

vector<Addr> SMS_Prefetcher::getRecordedPattern(AccessStat stat, int index) { 
    HistoryEntry entry = page_history_table.at(index);
    vector<Addr> addresses;
    Addr baseAddr = getRegion(stat.mem_addr);
    //printf("\nRecorded Pattern = %s", entry.spatial_pattern.to_string().c_str());
    //printf("\nEntry offset = %lu\n", entry.offset,
    for(size_t i = 0; i < entry.spatial_pattern.size(); i++){
	if(entry.spatial_pattern.test(i)){
		Addr memAddr = baseAddr | (i * BLOCK_SIZE);
		addresses.push_back(memAddr);
	}
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
	//printf( "Access was in filter table, discarding\n");
	GenerationEntry emptyEntry;
	filter_table.at(index) = emptyEntry;
	return;
    }
    index = findGeneration(stat, accumulation_table);
    if ( index != -1){
      
      	GenerationEntry entry = accumulation_table.at(index);
	accumulation_table.at(index) = GenerationEntry();
	HistoryEntry hentry(entry.pc, entry.offset);
	hentry.spatial_pattern = entry.pattern;
	
	//printf( "Inserting into page history table at index %d\n with pattern %s", index, hentry.spatial_pattern.to_string().c_str());
	page_history_table.at(history_table_index) = hentry;
	history_table_index = (history_table_index + 1) % history_table_size; 
      
    }
}

void SMS_Prefetcher::addToRecording(AccessStat stat) { 
    int index = findGeneration(stat, filter_table);
    if (index != -1){
	GenerationEntry entry = filter_table.at(index);
	if( entry.offset != getOffset(stat.mem_addr)){
	    //printf( "Second access in region, moving to accumulation table\n");
	    // second access within region, move to accumulation table
	    filter_table.at(index) = GenerationEntry();
	    entry.pattern |= (1 << entry.offset);
	    entry.pattern |= (1 << getOffset(stat.mem_addr));
	    insertGeneration(entry, accumulation_table, accumulation_table_index, accumulation_table_size);
	    accumulation_table_index = (accumulation_table_index + 1 ) % accumulation_table_size;
	}
	return;

    }
    index = findGeneration( stat, accumulation_table);
    if(index != -1){
	//printf( "Updating bit pattern \n");
	// update bit pattern
	accumulation_table.at(index).pattern |= (1 << getOffset(stat.mem_addr)); 
    }
    
}

/* the Spatial Memory Streaming prefetcher */
static SMS_Prefetcher sms_prefetcher;
static GHB_PCDC<256> ghb_prefetcher;

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
    PrefetchDecision sms_decision = sms_prefetcher.react_to_access(stat);
    PrefetchDecision ghb_decision = ghb_prefetcher.react_to_access(stat);

    std::set<Addr> prefetch_addresses;
    prefetch_addresses.insert(sms_decision.prefetchAddresses.begin(),
			      sms_decision.prefetchAddresses.end());

    prefetch_addresses.insert(ghb_decision.prefetchAddresses.begin(),
			      ghb_decision.prefetchAddresses.end());

    //printf( "Decided to prefetch %lu addresses\n", d.prefetchAddresses.size());

    typedef std::set<Addr>::const_iterator It;
    for (It it = prefetch_addresses.begin(),
             e = prefetch_addresses.end(); it != e; ++it) 
    {
      //printf("Considering prefetching address %lu\n", *it);
        if (!in_cache(*it) && !in_mshr_queue(*it))
        {
            //Should include check for *it <= MAX_PHYS_MEM_ADDR,
            //but keeping it unchecked to get crashes if it happens.
            //NOT
            if (*it <= MAX_PHYS_MEM_ADDR)
            {
		//printf("Prefetching address %lu\n", *it);
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
