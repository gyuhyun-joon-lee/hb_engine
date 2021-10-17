#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR

#include <stdint.h>
#include <limits.h>
#include <float.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef int32_t b32;

typedef uint8_t u8; 
typedef uint16_t u16; 
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t uintptr;

typedef float r32;
typedef double r64;

#define Assert(Expression) if(!(Expression)) {int *a = 0; *a = 0;}
#define ArrayCount(Array) (sizeof(Array) / sizeof(Array[0]))

#define global_variable static
#define local_variable static
#define internal static

#include "meka_platform_independent.h"
#include "meka_vulkan_function_loader.h"

global_variable b32 isEngineRunning;

#include "meka_vulkan.cpp"

internal
WRITE_CONSOLE(Win32WriteConsole)
{
    // TODO(joon) : is it better to just pass the console handle?
    Assert(console);
    WriteConsole(console, buffer, numberOfCharsToWrite, 0, 0);
    char pad[] = "\n\n";
    WriteConsole(console, pad, ArrayCount(pad), 0, 0);
}

internal
PLATFORM_READ_FILE(Win32ReadFile)
{
    platform_read_file_result result = {};

    HANDLE file = CreateFileA(filename,   
                            GENERIC_READ, 
                            FILE_SHARE_READ, 
                            0,
                            OPEN_EXISTING, 
                            FILE_ATTRIBUTE_NORMAL, 
                            0);
    if(file)
    {
        LARGE_INTEGER fileSize = {};
        if(GetFileSizeEx(file, &fileSize))
        {
            result.size = (u32)fileSize.QuadPart;
            result.memory = (u8 *)malloc(result.size);

            DWORD bytesRead = 0; 
            // TODO(joon) : add support for a file with a size bigger than 32 bit?
            if(ReadFile(file, result.memory, (u32)fileSize.QuadPart, &bytesRead, 0))
            {
                Assert((DWORD)fileSize.QuadPart == bytesRead);
            }
            else
            {
                DWORD error = GetLastError();
                // TODO(joon) : log
                Assert(0);
            }
        }
    }

    Assert(result.memory && result.size != 0);

    return result;
}

internal
PLATFORM_FREE_FILE_MEMORY(Win32FreeFileMemory)
{
    free(memory);
}


#undef internal 
#define internal static
#define MAX_CONSOLE_LINES 5000
internal HANDLE
Win32CreateConsoleWindow()
{
    HANDLE consoleHandle = 0;

    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if(AllocConsole())
    {
        consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if(GetConsoleScreenBufferInfo(consoleHandle, &consoleInfo))
        {
            consoleInfo.dwSize.Y = MAX_CONSOLE_LINES;
            SetConsoleScreenBufferSize(consoleHandle, consoleInfo.dwSize);
            // TODO(joon) : Best thing to do will be just redirecting the std out
            // to our console, but that does not work for some reasons. 
        }
    }
    else
    {
        // TODO(joon) : This should not be an assert, user should be able to
        // launch the application without the debug console!
        Assert(0);
    }

    return consoleHandle;
}

LRESULT CALLBACK 
MainWindowProcedure(HWND windowHandle,
				UINT message,
				WPARAM wParam,
				LPARAM lParam)
{
	LRESULT result = {};

	switch(message)
	{
		case WM_CREATE:
		{
		}break;

		case WM_PAINT:
		{
			HDC deviceContext = GetDC(windowHandle);

			if(deviceContext)
			{
				RECT clientRect = {};
				GetClientRect(windowHandle, &clientRect);

				PAINTSTRUCT paintStruct = {};
				paintStruct.hdc = deviceContext;
				// TODO : Find out whether this matters
				paintStruct.fErase = false;
				paintStruct.rcPaint = clientRect;

				BeginPaint(windowHandle, &paintStruct);
				EndPaint(windowHandle, &paintStruct);

				ReleaseDC(windowHandle, deviceContext);
			}
		}break;
		case WM_DESTROY:
		case WM_QUIT:
		{
		}break;

		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		{
            Assert(0);
		}break;

		default:
		{
			result = DefWindowProc(windowHandle, message, wParam, lParam);
		};
	}

	return result;
}

int CALLBACK 
WinMain(HINSTANCE instanceHandle,
		HINSTANCE prevInstanceHandle, 
		LPSTR cmd, 
		int cmdShowMethod)
{

	i32 screenWidth = 960;
	i32 screenHeight = 540;
	//Win32ResizeBuffer(&globalScreenBuffer, screenWidth, screenHeight);

    WNDCLASSEXA windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowProcedure;
    windowClass.hInstance = instanceHandle;
    windowClass.lpszClassName = "mekaWindowClass";
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	if(RegisterClassExA(&windowClass))
	{
		HWND windowHandle = CreateWindowExA(0,
										windowClass.lpszClassName,
								  		"meka_renderer",
							            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
							            CW_USEDEFAULT, CW_USEDEFAULT,
								  		screenWidth, screenHeight,
										0, 0,
										instanceHandle,
										0);
		if(windowHandle)
		{
            write_console_info writeConsoleInfo = {};
            writeConsoleInfo.console = Win32CreateConsoleWindow();
            writeConsoleInfo.WriteConsole = Win32WriteConsole;

            platform_api platformApi = {};
            platformApi.ReadFile = Win32ReadFile;
            platformApi.FreeFileMemory = Win32FreeFileMemory;

            Win32LoadVulkanLibrary("vulkan-1.dll");
            Win32InitVulkan(instanceHandle, windowHandle, screenWidth, screenHeight, &writeConsoleInfo, &platformApi);
            
            isEngineRunning = true;
            while(isEngineRunning)
            {
            }
        }
    }

    return 0;
}
