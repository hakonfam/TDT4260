#ifndef _ADAPTION_DECORATOR_H_
#define _ADAPTION_DECORATOR_H_

#include "prefetcher.hh"

#include <vector>

class AdaptionDecorator : public Prefetcher
{
public:
    explicit AdaptionDecorator(Prefetcher *p) 
        : p_(p), attempts_(0), hits_(0), misses_(0) { /* empty */ }
    virtual unsigned int prefetch_attempts() const { 
        return attempts_; 
    }
    virtual unsigned int prefetch_hits() const {
        return hits_; 
    }
    virtual void increase_aggressiveness() {
        p_->increase_aggressiveness();
    }
    virtual void decrease_aggressiveness() {
        p_->decrease_aggressiveness();
    }
    virtual PrefetchDecision react_to_access(AccessStat stat);
    virtual void prefetch_complete(Addr addr);
private:
    bool too_aggressive() const;
    bool too_defensive() const;
    Prefetcher *p_;
    unsigned int attempts_, hits_;
    unsigned long misses_;
};

PrefetchDecision AdaptionDecorator::react_to_access(AccessStat stat)
{
    if (stat.miss) 
    {
        misses_++;
    }
    if (get_prefetch_bit(stat.mem_addr))
    {
        hits_++;
        clear_prefetch_bit(stat.mem_addr);
    }

    if (too_aggressive())
    {
        increase_aggressiveness();
    }
    else
    {
        decrease_aggressiveness();
    }

    PrefetchDecision d = p_->react_to_access(stat);
    attempts_ += d.prefetchAddresses.size();
    return d;
}

void AdaptionDecorator::prefetch_complete(Addr addr)
{
    set_prefetch_bit(addr);
}

bool AdaptionDecorator::too_aggressive() const
{
    //TODO: Find a heuristic for determining whether 
    //aggressiveness should be decreased
    //Maybe low accuracy? ?_?
    double accuracy = static_cast<double>(hits_) / (attempts_ + hits_ + 1);
    double attemptRate = static_cast<double>(attempts) / (misses_ + 1);
    return accuracy < 0.25 && attemptRate > 0.75;
}

bool AdaptionDecorator::too_defensive() const
{
    //TODO: Find a heuristic for determining whether 
    //aggressiveness should be increased
    double accuracy = static_cast<double>(hits_) / (attempts_ + hits_ + 1);
    double attemptRate = static_cast<double>(attempts) / (misses_ + 1);
    return accuracy > 0.75 || (accuracy > 0.50 && attemptRate < 0.05);
}

#endif /* _ADAPTION_DECORATOR_H_ */
