#ifndef _GHB_PCDC_H_
#define _GHB_PCDC_H_

#include "prefetcher.hh"

#include <vector>

struct TableEntry
{
    explicit TableEntry() : address(0x0), previousMiss(0) { /* empty */ }
    explicit TableEntry(Addr addr, TableEntry *prevMiss)
        : address(addr), previousMiss(prevMiss) { /*empty */}
    Addr address;
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
        : head_(0), evictingOldEntry_(false) { /*empty */}
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
    for (int i = head_; i >= 0; i--)
    {
        if (buffer_[i].previousMiss == e)
        {
            return &buffer_[i];
        }
    }
    for (int i = TableSize - 1; i > head_; i--)
    {
        if (buffer_[i].previousMiss == e)
        {
            return &buffer_[i];
        }
    }
    return 0;
}


template <unsigned int TableSize>
TableEntry* GlobalHistoryBuffer<TableSize>::insert(
    const AccessStat &stat,
    TableEntry *previousMiss)
{
    if (evictingOldEntry_)
    {
        TableEntry *e = findFirstEntryReferencing(&buffer_[head_]);
        if (e != 0)
        {
            e->previousMiss = 0;
        }
        //If we are kicking out the previous miss, do not link to it...
        if (&buffer_[head_] == previousMiss)
        {
            previousMiss = 0;
        }
    }
    buffer_[head_] = TableEntry(stat.mem_addr, previousMiss);
    TableEntry *newEntry = &buffer_[head_];
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
    std::size_t index = pc % TableSize;
    buffer_[index] = TableEntry(pc, e);
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
          indexTable_() { /*empty */}
    unsigned int prefetch_attempts() const { return attempts_; }
    unsigned int prefetch_hits() const { return hits_; }
    void increase_aggressiveness() {
        if (numBlocksToPrefetch_ > 1)
            numBlocksToPrefetch_--;
    }
    void decrease_aggressiveness() {
        numBlocksToPrefetch_++;
    }
    PrefetchDecision react_to_access(AccessStat stat);
private:
    void insert(const AccessStat &stat);
  
    void compute_delta_table(
        const AccessStat &stat,
        std::vector<int> &deltaTable) const;

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
void GHB_PCDC<TableSize>::compute_delta_table(
    const AccessStat &stat,
    std::vector<int> &deltaTable) const
{
    for (TableEntry *e = indexTable_.previousAccessTo(stat.pc);
         e->previousMiss != 0; e = e->previousMiss)
    {
        deltaTable.push_back(e->address - e->previousMiss->address);
    }
}

template<unsigned int TableSize>
int GHB_PCDC<TableSize>::pastPreviousOccurrenceOfLastPair(
    const std::vector<int> &deltas) const
{
    if (deltas.size() < 2) return -1;

    int d1 = deltas[deltas.size() - 2], d2 = deltas[deltas.size() - 1];
    for (int i = deltas.size() - 4; i >= 0; i--)
    {
        if (deltas[i] == d1 && deltas[i+1] == d2)
            return i + 2;
    }
    return -1;
}


template<unsigned int TableSize>
PrefetchDecision GHB_PCDC<TableSize>::react_to_access(AccessStat stat)
{
    insert(stat);
    std::vector<int> deltas;
    compute_delta_table(stat, deltas);
    DPRINTF(HWPrefetch, "Size of delta table: %u\n", deltas.size());
    int index = pastPreviousOccurrenceOfLastPair(deltas);
    if (index == -1)
    {
        return PrefetchDecision(); //Do not prefetch
    }
    else
    {
        std::vector<Addr> addrs;
        for (int i = index; i < index + numBlocksToPrefetch_; ++i)
        {
            addrs.push_back(stat.mem_addr + deltas[i]);
        }
        return PrefetchDecision(addrs);
    }
}

#endif /* _GHB_PCDC_H_ */

