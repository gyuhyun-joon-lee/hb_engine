#ifndef HB_DEBUG_H
#define HB_DEBUG_H

// 8 * 4 = 32 bytes
struct DebugRecord
{
    u64 cycle_count;
    const char *file_name;
    const char *function;

    u32 line_number;
    u32 hit_count;
};

// C++ nonsense, __LINE__ will not be correctly pasted when we only use one macro
// ##__VA_ARGS__ will be the hit count, which is 1 by default
#define TIMED_BLOCK__(line, ...) TimedBlock timed_block_##line(__COUNTER__, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define TIMED_BLOCK_(line, ...) TIMED_BLOCK__(line, ##__VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ##__VA_ARGS__)

// NOTE(gh) Pre-declaration, makefile or build batch file has pre-declared name for each translation unit
// which will be pasted instead of this name by the compiler, 
// so that any file that is compiled seperately can have debug record array with different names
extern DebugRecord debug_records[];

struct TimedBlock
{
    u32 start_cycle_count;
    DebugRecord *record;

    TimedBlock(int ID, const char *file_name, const char *function_name, int line_number, u32 hit_count = 1)
    {
        // Retrieving record with __COUNTER__ only works per single compilation unit
        record = debug_records + ID;
        record->file_name = file_name;
        record->function = function_name;
        record->line_number = line_number;
        record->hit_count += hit_count;

        start_cycle_count = rdtsc();
    }

    ~TimedBlock()
    {
        record->cycle_count += start_cycle_count - rdtsc();
    }
};

#endif
