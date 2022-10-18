#ifndef HB_DEBUG_H
#define HB_DEBUG_H

struct DebugRecord
{
    const char *file;
    const char *function;
    u32 line;

    // NOTE(gh) (hit_count << 32) | (cycle_count), we can decrease the size of hit_count for more cycle_count
    // This helps us to use only one atomic operation to modify this value
    volatile u64 hit_count_cycle_count;
};

#if HB_DEBUG
// C++ nonsense, __LINE__ will not be correctly pasted when we only use one macro
// ##__VA_ARGS__ will be the hit count, which is 1 by default
#define __TIMED_BLOCK(line, ...) TimedBlock timed_block_##line(__COUNTER__, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define _TIMED_BLOCK(line, ...) __TIMED_BLOCK(line, ##__VA_ARGS__)
#define TIMED_BLOCK(...) _TIMED_BLOCK(__LINE__, ##__VA_ARGS__)
#else

#define TIMED_BLOCK(...) 
#endif

// NOTE(gh) Pre-declaration, makefile or build batch file has pre-declared name for each translation unit
// which will be pasted instead of this name by the compiler, 
// so that any file that is compiled seperately can have debug record array with different names
extern DebugRecord debug_records[];

struct TimedBlock
{
    u64 start_cycle_count;
    u32 hit_count;
    DebugRecord *record;

    TimedBlock(int ID, const char *file, const char *function, int line, u32 hit_count_init = 1)
    {
        // Retrieving record with __COUNTER__ only works per single compilation unit
        record = debug_records + ID;
        record->file = file;
        record->function = function;
        record->line = line;
        hit_count = hit_count_init;

        start_cycle_count = rdtsc();
    }

    ~TimedBlock()
    {
        u64 hit_count_cycle_count = ((u64)hit_count << 32) | (rdtsc() - start_cycle_count);
        // TODO(gh) Because of this, you should never put timed_block inside the very hot loop.
        // Instead, put it outside the loop and do hit_count += loop_count
        atomic_add_64(&record->hit_count_cycle_count, hit_count_cycle_count);
    }
};

#endif
