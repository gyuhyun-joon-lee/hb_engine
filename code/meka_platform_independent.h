#ifndef MEKA_PLATFORM_INDEPENDENT_H
#define MEKA_PLATFORM_INDEPENDENT_H

#define WRITE_CONSOLE(name) void (name)(void *console, const char *buffer, u32 numberOfCharsToWrite)
typedef WRITE_CONSOLE(write_console);

struct write_console_info
{
    void *console; // console handle
    write_console *WriteConsole; // function pointer
};

struct platform_read_file_result
{
    u8 *memory;
    u32 size;
};
#define PLATFORM_READ_FILE(name) platform_read_file_result (name)(char *filename)
typedef PLATFORM_READ_FILE(platform_read_file);

#define PLATFORM_FREE_FILE_MEMORY(name) void (name)(void *memory)
typedef PLATFORM_FREE_FILE_MEMORY(platform_free_file_memory);

struct platform_api
{
    platform_read_file *ReadFile;
    platform_free_file_memory *FreeFileMemory;
};

internal u32
GetCharCountNullTerminated(const char *buffer)
{
    u32 result = 0;
    if(buffer)
    {
        while(buffer[result] != '\0')
        {
            ++result;
        }
    }

    return result;
}

// TODO(joon) : inefficient for long char arrays
internal b32
StringCompare(char *a, char *b)
{
    b32 result = true;
    u32 aCount = GetCharCountNullTerminated(a);
    u32 bCount = GetCharCountNullTerminated(b);

    if(aCount == bCount)
    {
        for(u32 i = 0;
            i < aCount;
            ++i)
        {
            if(a[i] != b[i])
            {
                result = false;
                break;
            }
        }
    }
    else
    {
        result = false;
    }

    return result;
}

#endif
