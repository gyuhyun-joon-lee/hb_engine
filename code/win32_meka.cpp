#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR

#include <stdint.h>
#include <limits.h>
#include <float.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 b3232;

typedef uint8_t uint8; 
typedef uint16_t uint16; 
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8 i8;
typedef int16 i16;
typedef int32 i32;
typedef int64 i64;
typedef int32 b32;

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

#include "meka_vulkan_function_loader.h"

#include "meka_vulkan.cpp"

global_variable b32 isEngineRunning;

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
    windowClass.lpszClassName = "mimicWindowClass";
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
	if(RegisterClassExA(&windowClass))
	{
		HWND windowHandle = CreateWindowExA(0,
										windowClass.lpszClassName,
								  		"mimic_renderer",
							            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
							            CW_USEDEFAULT, CW_USEDEFAULT,
								  		screenWidth, screenHeight,
										0, 0,
										instanceHandle,
										0);
		if(windowHandle)
		{
            Win32LoadVulkanLibrary("vulkan-1.dll");
            InitVulkanWin32(instanceHandle, windowHandle, screenWidth, screenHeight);
            
            isEngineRunning = true;
            while(isEngineRunning)
            {
            }
        }
    }

    return 0;
}
