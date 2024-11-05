#ifndef INLINE_SCHEDULER_H
#define INLINE_SCHEDULER_H

struct InlineScheduler {
    bool try_schedule(auto&&){
        return false;
    }  
};

#endif//INLINE_SCHEDULER_H

