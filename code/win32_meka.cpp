/*
 * Written by Gyuhyun 'Joon' Lee
 * https://github.com/meka-lopo/
 */
#define VK_USE_PLATFORM_WIN32_KHR
//#define VK_USE_MESH_SHADER

#include "meka_platform.h"
#include "meka_vulkan_function_loader.h"
#include "meka_vulkan.cpp"
#include "meka_render.h"
#include "meka_mesh_loader.cpp"

#define CGLTF_IMPLEMENTATION
#include "../external_libraries/cgltf.h"

global_variable b32 isEngineRunning;
global_variable b32 is_w_down;
global_variable b32 is_a_down;
global_variable b32 is_s_down;
global_variable b32 is_d_down;

#include <windows.h>
#include <time.h>
#include <stdlib.h>

#ifndef _FILE_DEFINED
struct _iobuf {
        char *_ptr;
        int   _cnt;
        char *_base;
        int   _flag;
        int   _file;
        int   _charbuf;
        int   _bufsiz;
        char *_tmpfname;
        };
typedef struct _iobuf FILE;
#define _FILE_DEFINED
#endif

#pragma comment (lib, "user32")
#pragma comment (lib, "winmm")
#pragma comment (lib, "gdi32")

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
                CloseHandle(file);
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
#include <io.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#define internal static
#define MAX_CONSOLE_LINES 5000
internal HANDLE
Win32CreateConsoleWindow()
{
    HANDLE consoleHandle = 0;
    HANDLE originalSTDHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if(AllocConsole())
    {
        consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if(GetConsoleScreenBufferInfo(consoleHandle, &consoleInfo))
        {
            consoleInfo.dwSize.Y = MAX_CONSOLE_LINES;
            SetConsoleScreenBufferSize(consoleHandle, consoleInfo.dwSize);

            HANDLE lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);

            long hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);

            FILE *fp = _fdopen( hConHandle, "w" );

            *stdout = *fp;
            setvbuf( stdout, NULL, _IONBF, 0 );

            std::ios::sync_with_stdio();
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
            isEngineRunning = false;
		}break;

		case WM_KEYUP:
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		{
            WPARAM keyCode = wParam;

            b32 wasDown = (((lParam >>30) & 1) == 1);
            b32 isDown = (((lParam >>31) & 1) == 0);

            if(wasDown != isDown)
            {
                if(keyCode == 'W')
                {
                    isWDown = isDown;
                }
                else if(keyCode == 'A')
                {
                    isADown = isDown;
                }
                else if(keyCode == 'S')
                {
                    isSDown = isDown;
                }
                else if(keyCode == 'D')
                {
                    isDDown = isDown;
                }
                else if(keyCode == VK_ESCAPE)
                {
                    isEngineRunning = false;
                }
            }
		}break;

		default:
		{
			result = DefWindowProc(windowHandle, message, wParam, lParam);
		};
	}

	return result;
}

internal render_mesh 
CreateVulkanRenderMeshFromLoadedMesh(renderer *renderer, loaded_raw_mesh *loadedMesh)
{
    Assert(loadedMesh->positionCount !=0);
    if(loadedMesh->normals)
    {
        Assert(loadedMesh->positionCount == loadedMesh->normalCount);
    }

    render_mesh result = {};
    result.indexCount = loadedMesh->indexCount;

    result.vertexBuffer = VkCreateHostVisibleCoherentBuffer(renderer->physicalDevice, renderer->device, 
                                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                    sizeof(vertex)*loadedMesh->positionCount);

    vertex *vertices = (vertex *)malloc(sizeof(vertex)*loadedMesh->positionCount);
    // re-construct the vertex as we want
    // TODO(joon) : instead of doing this, use SOA?
    // NOTE(joon) : Only possible because this is host visible 
    for(u32 vertexIndex = 0;
        vertexIndex < loadedMesh->positionCount;
        ++vertexIndex)

    {
        vertex *vert = vertices + vertexIndex;
        vert->p = loadedMesh->positions[vertexIndex];
        if(loadedMesh->normals)
        {
            vert->normal = loadedMesh->normals[vertexIndex];
        }
    }
    VkCopyMemoryToHostVisibleCoherentBuffer(&result.vertexBuffer, 0, vertices, result.vertexBuffer.size);

    result.indexBuffer = VkCreateHostVisibleCoherentBuffer(renderer->physicalDevice, renderer->device, 
                                                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                    sizeof(u16)*loadedMesh->indexCount);
    VkCopyMemoryToHostVisibleCoherentBuffer(&result.indexBuffer, 0, loadedMesh->indices, result.indexBuffer.size);

    free(vertices);

    return result;
}

internal void
RenderMesh(renderer *renderer, render_mesh *renderMesh, VkCommandBuffer commandBuffer)
{
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = renderMesh->vertexBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = renderMesh->vertexBuffer.size;

    VkWriteDescriptorSet writeDescriptorSet[1] = {};
    writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet[0].dstSet = 0;
    writeDescriptorSet[0].dstBinding = 0;
    writeDescriptorSet[0].descriptorCount = 1;
    writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writeDescriptorSet[0].pBufferInfo = &bufferInfo;

    vkCmdPushDescriptorSetKHR(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                            renderer->pipelineLayout, 0, ArrayCount(writeDescriptorSet), writeDescriptorSet);

    vkCmdBindIndexBuffer(commandBuffer, renderMesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(commandBuffer, renderMesh->indexCount, 1, 0, 0, 0);
}

global_variable HANDLE globalSemaphore;
struct win32_thread
{
    DWORD ID;
    // TODO(joon) : This should be platform independent!
    thread_work_queue *queue;
};

#include <intrin.h>
// NOTE(joon) : x64(lfence sfece) ARM(dmb for load and dsb for store)
#define WriteBarrier /*prevent compilation reordering*/_WriteBarrier(); /*prevent excute order change by the processor, only needed for special types of memories such as write combine memory*///_mm_sfence(); 
#define ReadBarrier _ReadBarrier();

// NOTE(joon) : use this to add what thread should do
internal 
THREAD_WORK_CALLBACK(PrintString)
{
    char *stringToPrint = (char *)data;
    printf("%s\n", stringToPrint);
}

// IMPORTANT(joon) : This is meant to be used for single producer, so there's no synchronization here
internal
PLATFORM_ADD_THREAD_WORK_QUEUE_ITEM(Win32AddThreadWorkQueueItem)
{
    // TODO(joon) : When we increment the entry count, thread can start working even if we hadn't fill the work
    thread_work_item *item = queue->items + queue->writeIndex;
    Assert(item->written == false);

    item->callback = threadWorkCallback;
    item->data = data;
    item->written = true;

    WriteBarrier;

    queue->writeIndex++;
    Assert(queue->writeIndex != queue->readIndex);

    ReleaseSemaphore(globalSemaphore, 1, 0);
}


internal b32
Win32DoThreadWorkItem(thread_work_queue *queue, u32 threadID)
{
    b32 didWork = false;

    if(queue->writeIndex != queue->readIndex)
    {
        LONG expectedCurrentValue = queue->readIndex;
        LONG updatedValue = expectedCurrentValue+1;
        LONG actualValue = InterlockedExchange((LONG volatile *)&queue->readIndex, updatedValue);

        if(actualValue == expectedCurrentValue)
        {
            // NOTE(joon) : The variable pointed to by the Addend parameter must be aligned on a 32-bit boundary
            // NOTE(joon) : This function is atomic with respect to calls to other interlocked functions, so each thread with this function is guaranteed to get different value
            u32 itemIndex = actualValue;
            thread_work_item *item = queue->items + itemIndex;
            Assert(item->written);

            item->callback(item->data);
            ReadBarrier;
            
            InterlockedExchange((LONG volatile *)&item->written, false);
            didWork = true;
        }
    }

    return didWork;
}

// if the thread work is time sensitive, we can use this to spinlock the main thread
// to wait for all thread works to be finished, while also doing the work itself
// IMPORTANT(joon) : This is meant to be used for single producer, so there's no synchronization here
internal 
PLATFORM_FINISH_ALL_THREAD_WORK_QUEUE_ITEMS(Win32FinishAllThreadWorkQueueItem)
{
    while(queue->readIndex != queue->writeIndex) 
    {
        Win32DoThreadWorkItem(queue, 0);
    }
}

internal DWORD 
ThreadProc(void *data)
{
    win32_thread *thread = (win32_thread *)data;
    thread_work_queue *queue = thread->queue;

    while(1)
    {
        if(Win32DoThreadWorkItem(queue, thread->ID))
        {
        }
        else
        {
            // NOTE(joon) : Wake up when the semaphore count not 0, and when it wakes up, decrement it by 1
            WaitForSingleObjectEx(globalSemaphore, INFINITE, 0);
        }
    }

    return 0;
}

// TODO(joon) : maket this to use random table
internal r32
random_between_0_1()
{
    r32 result = (r32)rand()/(r32)RAND_MAX;
    return result;
}

internal r32
random_between(r32 min, r32 max)
{
    return min + (max-min)*random_between_0_1();
}

internal r32
random_between_minus_1_1()
{
    r32 result = 2.0f*((r32)rand()/(r32)RAND_MAX) - 1.0f;
    return result;
}

inline r32
random_range(r32 value, r32 range)
{
    r32 half_range = range/2.0f;

    r32 result = value + half_range*random_between_minus_1_1();

    return result;
}

inline i32
round_r32_i32(r32 value)
{
    // TODO(joon) : intrinsic?
    return (i32)roundf(value);
}


internal void
make_mountain_inside_terrain(loaded_raw_mesh *terrain, 
                            i32 x_count, i32 z_count,
                            r32 center_x, r32 center_z, 
                            i32 stride,
                            r32 dim, r32 radius, r32 max_height)
{
    i32 min_x = maximum(round_r32_i32((center_x - radius)/dim), 0);
    i32 max_x = minimum(round_r32_i32((center_x + radius)/dim), x_count);

    i32 min_z = maximum(round_r32_i32((center_z - radius)/dim), 0);
    i32 max_z = minimum(round_r32_i32((center_z + radius)/dim), z_count);
    
    i32 center_x_i32 = (i32)((max_x + min_x)/2.0f);
    i32 center_z_i32 = (i32)((max_z + min_z)/2.0f);

    v3 *row = terrain->positions + min_z*stride + min_x;
    for(i32 z = min_z;
            z < max_z;
            z++)
    {
        v3 *p = row;

        r32 y = 0.0f;
        for(i32 x = min_x;
            x < max_x;
            x++)
        {
            r32 distance = dim*Length(V2i(center_x_i32, center_z_i32) - V2i(x, z));
            if(distance <= radius)
            {
                r32 height = (1.0f - (distance / radius))*max_height;
                p->y = random_range(height, 0.45f*height);
                //p->y = height*random_between_0_1();
            }

            p++;
        }

        row += stride;
    }
}

int CALLBACK 
WinMain(HINSTANCE instanceHandle,
		HINSTANCE prevInstanceHandle, 
		LPSTR cmd, 
		int cmdShowMethod)
{
    srand((u32)time(NULL));

#if MEKA_DEBUG
    Win32CreateConsoleWindow();
#endif

    win32_thread threads[8] = {};

    globalSemaphore = CreateSemaphoreExA(0, 0, ArrayCount(threads), 0, 0, SEMAPHORE_ALL_ACCESS);

    thread_work_queue queue = {};

    for(u32 threadIndex = 0;
        threadIndex < ArrayCount(threads);
        ++threadIndex)
    {
        win32_thread *thread = threads + threadIndex;
        thread->ID = threadIndex + 1; // 0th thread is the main thread
        thread->queue = &queue;

        // TODO(joon) : Any reason to store this value? i.e. debug purpose?
        DWORD threadID;
        HANDLE result = CreateThread(0, 0, ThreadProc, (void *)(threads+threadIndex), 0, &threadID); 
    }

#if 0

    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A0");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A1");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A2");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A3");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A4");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A5");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A6");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A7");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A8");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String A9");

    Sleep(1000);
    
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B0");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B1");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B2");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B3");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B4");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B5");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B6");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B7");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B8");
    Win32AddThreadWorkQueueItem(&queue, PrintString, "String B9");

    Win32FinishAllThreadWorkQueueItem(&queue);
#endif

	i32 windowWidth = 960;
	i32 windowHeight = 540;
    r32 windowWidthOverHeight = (r32)windowWidth/(r32)windowHeight;
	//Win32ResizeBuffer(&globalScreenBuffer, windowWidth, windowHeight);

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
								  		windowWidth, windowHeight,
										0, 0,
										instanceHandle,
										0);
		if(windowHandle)
		{
            i32 screenWidth = GetSystemMetrics(SM_CXFULLSCREEN);
            i32 screenHeight = GetSystemMetrics(SM_CYFULLSCREEN);


            platform_api platformApi = {};
            platformApi.ReadFile = Win32ReadFile;
            platformApi.FreeFileMemory = Win32FreeFileMemory;

            platform_memory platformMemory = {};
            platformMemory.size = Gigabytes(1);
            platformMemory.base = VirtualAlloc(
#if 0
                    (void *)(0 + Terabytes(2)), 
#else
                    0,
#endif
                    (size_t)platformMemory.size,
                    MEM_COMMIT|MEM_RESERVE,
                    PAGE_READWRITE);

            //loaded_raw_mesh mesh = ReadSingleMeshOnlygltf(&platformApi, "../textures/cube.gltf", "../textures/cube.bin");
            //loaded_raw_mesh mesh = ReadSingleMeshOnlygltf(&platformApi, "../textures/BarramundiFish/BarramundiFish.gltf", "../textures/BarramundiFish/BarramundiFish.bin");
            //loaded_raw_mesh mesh = ReadSingleMeshOnlygltf(&platformApi, "../textures/damaged_helmet/DamagedHelmet.gltf", "../textures/damaged_helmet/DamagedHelmet.bin");
            //OptimizeMeshGH(&mesh, 0.5f);

            loaded_raw_mesh mesh = {};

            platform_read_file_result binFile = Win32ReadFile("../textures/damaged_helmet/DamagedHelmet.bin");

            cgltf_options options = {cgltf_file_type_gltf};
            cgltf_data* data = NULL;
            cgltf_result result = cgltf_parse_file(&options, "../textures/damaged_helmet/DamagedHelmet.gltf", &data);
            if (result == cgltf_result_success)
            {
                /* TODO make awesome stuff */
                for(u32 meshIndex = 0;
                    meshIndex < data->meshes_count;
                    ++meshIndex)
                {
                    cgltf_mesh *cgMesh = data->meshes + meshIndex;
                    cgltf_primitive *cgPrimitives = cgMesh->primitives;
                    size_t indexCount = cgPrimitives->indices->count;
                    size_t indexOffset = cgPrimitives->indices->buffer_view->offset;
                    size_t indexSize = cgPrimitives->indices->buffer_view->size;

                    Assert(indexCount < U32_MAX);

                    mesh.indexCount = (u32)indexCount;
                    mesh.indices = PushArray(&platformMemory, u16, mesh.indexCount);
                    memcpy(mesh.indices, binFile.memory + indexOffset, indexSize);

                    for(u32 attributeIndex = 0;
                        attributeIndex < cgPrimitives->attributes_count;
                        ++attributeIndex)
                    {
                        cgltf_attribute *attribute = cgPrimitives->attributes + attributeIndex;
                        size_t offset = attribute->data->buffer_view->offset;
                        size_t size = attribute->data->buffer_view->size;
                        size_t count = attribute->data->count;

                        Assert(count < U32_MAX);

                        switch(attribute->type)
                        {
                            case cgltf_attribute_type_position:
                            {
                                Assert(attribute->data->type == cgltf_type_vec3 && attribute->data->stride == 12);

                                mesh.positionCount = (u32)count;
                                mesh.positions = PushArray(&platformMemory, v3, mesh.positionCount);
                                memcpy(mesh.positions, binFile.memory + offset, size);
                            }break;
                            case cgltf_attribute_type_normal:
                            {
                                Assert(attribute->data->type == cgltf_type_vec3 && attribute->data->stride == 12);

                                mesh.normalCount = (u32)count;
                                mesh.normals = PushArray(&platformMemory, v3, mesh.normalCount);
                                memcpy(mesh.normals, binFile.memory + offset, size);
                            }break;
                            //case cgltf_attribute_type_tangent:
                            //case cgltf_attribute_type_color:
                        }
                    }
                }
                int a = 1;
                cgltf_free(data);
            }
            Win32FreeFileMemory(binFile.memory);

            // 100 vertices means there will be 99 quads per line
            u32 zCount = 100;
            u32 xCount = 100;
            loaded_raw_mesh terrain = {};
            terrain.positionCount = (zCount) * (xCount);
            terrain.positions = PushArray(&platformMemory, v3, terrain.positionCount);

            r32 startingX = 0;
            r32 startingZ = 0;
            r32 dim = 5;
            for(u32 z = 0;
                z < zCount;
                ++z)
            {
                for(u32 x = 0;
                    x < xCount;
                    ++x)
                {
                    r32 randomY = random_between_minus_1_1();
                    r32 yRange = 2.f;
                    v3 p = V3(startingX + x*dim, yRange*randomY, startingZ + z*dim);
                    terrain.positions[z*xCount + x] = p;
                }
            }
            
            for(u32 mountain_index = 0;
                    mountain_index < 12;
                    mountain_index++)
            {
                r32 x = random_between(0, xCount*dim);
                r32 z = random_between(0, zCount*dim);
                
                r32 height = random_between(20, 100);
                r32 radius = random_between(10, 100);
                make_mountain_inside_terrain(&terrain, 
                        xCount, zCount, 
                        x, z, xCount,
                        dim, radius, height);
            }

            terrain.indexCount = 2*3*(zCount - 1)*(xCount - 1);
            terrain.indices = PushArray(&platformMemory, u16, terrain.indexCount);

            terrain.normalCount = terrain.positionCount;
            terrain.normals = PushArray(&platformMemory, v3, terrain.normalCount);

            u32 indexIndex = 0;
            for(u32 z = 0;
                z < zCount-1;
                ++z)
            {
                for(u32 x = 0;
                    x < xCount-1;
                    ++x)
                {
                    u32 startingIndex = z*zCount + x;


                    terrain.indices[indexIndex++] = (u16)startingIndex;
                    terrain.indices[indexIndex++] = (u16)(startingIndex+xCount);
                    terrain.indices[indexIndex++] = (u16)(startingIndex+xCount+1);

                    terrain.indices[indexIndex++] = (u16)startingIndex;
                    terrain.indices[indexIndex++] = (u16)(startingIndex+xCount+1);
                    terrain.indices[indexIndex++] = (u16)(startingIndex+1);

                    Assert(indexIndex <= terrain.indexCount);
                }
            }
            Assert(indexIndex == terrain.indexCount);

            struct vertex_normal_hit
            {
                v3 normalSum;
                u32 hit;
            };

            temp_memory meshContructionTempMemory = StartTempMemory(&platformMemory, Megabytes(16));
            vertex_normal_hit *normalHits = PushArray(&meshContructionTempMemory, vertex_normal_hit, terrain.positionCount);

            for(u32 i = 0;
                    i < terrain.indexCount;
                    i += 3)
            {
                u32 i0 = terrain.indices[i];
                u32 i1 = terrain.indices[i+1];
                u32 i2 = terrain.indices[i+2];

                v3 v0 = terrain.positions[i0] - terrain.positions[i1];
                v3 v1 = terrain.positions[i2] - terrain.positions[i1];

                v3 normal = Normalize(Cross(v1, v0));

                normalHits[i0].normalSum += normal;
                normalHits[i0].hit++;
                normalHits[i1].normalSum += normal;
                normalHits[i1].hit++;
                normalHits[i2].normalSum += normal;
                normalHits[i2].hit++;
            }

            for(u32 normalIndex = 0;
                    normalIndex < terrain.normalCount;
                    ++normalIndex)
            {
                vertex_normal_hit *normalHit = normalHits + normalIndex;
                terrain.normals[normalIndex] = normalHit->normalSum/(r32)normalHit->hit;
            }
            EndTempMemory(&meshContructionTempMemory);

            renderer renderer = {};

            Win32LoadVulkanLibrary("vulkan-1.dll");
            Win32InitVulkan(&renderer, instanceHandle, windowHandle, windowWidth, windowHeight, &platformApi, &platformMemory);
#if 0

            // NOTE(joon) : nvidia says we should not have more than 64 vertices & 126 indices
            // as the given buffer is only 128 bytes
            struct meshlet
            {
                u32 vertices[64];

                // TODO(joon) : This will be just sequential indices for now, but when we make the vertex buffer more optimal,
                // we need to make this work like a real index buffer that we were using
                u8 indices[126]; // index to the vertices above, not the actual vertices
                u8 triangleCount;
                u8 vertexCount;
            };

            meshlet meshlet = {}; 
            for(u32 indexIndex = 0;
                indexIndex < mesh.indexCount;
                ++indexIndex)
            {
                mesh.indices[indexIndex + 0];
                mesh.indices[indexIndex + 1];
                mesh.indices[indexIndex + 2];
            }
#endif
            printf("vulkan initialized");

            render_mesh terrainRenderMesh = CreateRenderMeshFromLoadedMesh(&renderer, &terrain);
            render_mesh helmetRenderMesh = CreateRenderMeshFromLoadedMesh(&renderer, &mesh);

            // IMPORTANT(joon): no rotation means camera's orientation is the same as the world coordinate
            // which means the forward vector is 0, 0, -1
            camera camera = {};
            camera.p = {0, 0, 1};

            u64 aligned_uniform_buffer_size = renderer.swapchainImageCount*(sizeof(uniform_buffer)/renderer.uniform_buffer_offset_alignment + 1)*renderer.uniform_buffer_offset_alignment;
            vk_host_visible_coherent_buffer uniformBuffer = VkCreateHostVisibleCoherentBuffer(renderer.physicalDevice, renderer.device, 
                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            aligned_uniform_buffer_size);

            // TODO(joon)VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
            for(u32 swapchainImageIndex = 0;
                swapchainImageIndex < renderer.swapchainImageCount;
                ++swapchainImageIndex)
            {
                VkCommandBuffer *commandBuffer = renderer.graphicsCommandBuffers + swapchainImageIndex;
                
                // TODO(joon) : fence needed here!
                VkCommandBufferBeginInfo commandBufferBeginInfo = {};
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                CheckVkResult(vkBeginCommandBuffer(*commandBuffer, &commandBufferBeginInfo));

                VkClearValue clearValues[2] = {}; 
                clearValues[0].color = {0.0001f, 0.0001f, 0.0001f, 1};
                clearValues[1].depthStencil.depth = 1.0f;
                //clearValues[1].depthStencil;
                VkRenderPassBeginInfo renderPassBeginInfo = {};
                renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassBeginInfo.renderPass = renderer.renderPass;
                renderPassBeginInfo.framebuffer = renderer.framebuffers[swapchainImageIndex];
                renderPassBeginInfo.renderArea.offset = {0, 0};
                renderPassBeginInfo.renderArea.extent.width = (u32)renderer.surfaceExtent.x;
                renderPassBeginInfo.renderArea.extent.height = (u32)renderer.surfaceExtent.y;
                renderPassBeginInfo.clearValueCount = ArrayCount(clearValues);
                renderPassBeginInfo.pClearValues = clearValues;
                vkCmdBeginRenderPass(*commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipeline);

                VkViewport viewport = {};
                viewport.width = renderer.surfaceExtent.x;
                viewport.height = renderer.surfaceExtent.y;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissorRect = {};
                scissorRect.extent.width = (u32)renderer.surfaceExtent.x;
                scissorRect.extent.height = (u32)renderer.surfaceExtent.y;

                vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(*commandBuffer, 0, 1, &scissorRect);

                VkWriteDescriptorSet writeDescriptorSet[1] = {};

                VkDescriptorBufferInfo uniformBufferInfo = {};
                uniformBufferInfo.buffer = uniformBuffer.buffer;

                u64 aligned_uniform_buffer_offset = swapchainImageIndex*(sizeof(uniform_buffer)/renderer.uniform_buffer_offset_alignment)*renderer.uniform_buffer_offset_alignment;
                uniformBufferInfo.offset = aligned_uniform_buffer_offset;
                uniformBufferInfo.range = sizeof(uniform_buffer);

                writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSet[0].dstSet = 0;
                writeDescriptorSet[0].dstBinding = 1;
                writeDescriptorSet[0].descriptorCount = 1;
                writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writeDescriptorSet[0].pBufferInfo = &uniformBufferInfo;

                vkCmdPushDescriptorSetKHR(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                        renderer.pipelineLayout, 0, ArrayCount(writeDescriptorSet), writeDescriptorSet);

                RenderMesh(&renderer, &helmetRenderMesh, *commandBuffer);

                RenderMesh(&renderer, &terrainRenderMesh, *commandBuffer);

                vkCmdEndRenderPass(*commandBuffer);
                CheckVkResult(vkEndCommandBuffer(*commandBuffer));
            }
            
            isEngineRunning = true;

            u32 currentFrameIndex = 0;

            int mousex = 0;
            int mousey = 0;
            POINT mouseP;
            GetCursorPos(&mouseP);
            mousex = (i32)mouseP.x;
            mousey = (i32)mouseP.y;

            int mouseDx = 0;
            int mouseDy = 0;

            while(isEngineRunning)
            {
                MSG message;
                while(PeekMessageA(&message, windowHandle, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                // NOTE(joon) : Based on top left corner
                RECT windowRect = {};
                GetClientRect(windowHandle, &windowRect);

                POINT windowPos = {};
                ClientToScreen(windowHandle, &windowPos);
                windowRect.left += windowPos.x;
                windowRect.right += windowPos.x;
                windowRect.top += windowPos.y;
                windowRect.bottom += windowPos.y;

                HWND activeWindow = GetFocus();
                if(activeWindow == windowHandle)
                {
                    ClipCursor(&windowRect);

                    POINT currentMouseP = {};
                    if(GetCursorPos(&currentMouseP))
                    {
                        mouseDx = (i32)currentMouseP.x-mousex;
                        mouseDy = (i32)currentMouseP.y-mousey;

                        if(currentMouseP.x == windowRect.left)
                        {
                            mouseDx = -8;
                        }
                        else if(currentMouseP.x == windowRect.right - 1)
                        {
                            mouseDx = 8;
                        }

                        if(currentMouseP.y == windowRect.top)
                        {
                            mouseDy = -8;
                        }
                        else if(currentMouseP.y == windowRect.bottom - 1)
                        {
                            mouseDy = 8;
                        }
                        //printf("ClientRect %i, %i", windowRect.left, windowRect.right);
                        //printf("ClientRect %i, %i\n", windowRect.top, windowRect.bottom);
                        //printf("%i, %i\n", currentMouseP.x, currentMouseP.y);

                        mousex = (i32)currentMouseP.x;
                        mousey = (i32)currentMouseP.y;
                    }
                }
                
                camera.alongY += -4.0f*(mouseDx/(r32)windowWidth);
                camera.alongX += -4.0f*(mouseDy/(r32)windowHeight);

                r32 camera_speed = 1.0f;

                v3 cameraDir = Normalize(QuarternionRotationV001(camera.alongZ, 
                                        QuarternionRotationV010(camera.alongY, 
                                         QuarternionRotationV100(camera.alongX, V3(0, 0, -1)))));
                // NOTE(joon) : For a rotation matrix, order is x->y->z 
                // which means the matrices should be ordered in z*y*x*vector
                // However, for the view matrix, because the rotation should be inversed,
                // it should be ordered in x*y*z!
                
                if(isWDown)
                {
                    camera.p += camera_speed*cameraDir;
                }
                
                if(isSDown)
                {
                    camera.p -= camera_speed*cameraDir;
                }


                // TODO(joon) : Check whether this fence is working correctly! only one fence might be enough?
#if 0
                if(vkGetFenceStatus(renderer.device, renderer.imageReadyToRenderFences[currentFrameIndex]) == VK_SUCCESS)
                {
                    // The image that we are trying to get might not be ready inside GPU, 
                    // and CPU can only know about it using fence.
                    vkWaitForFences(renderer.device, 1, &renderer.imageReadyToRenderFences[currentFrameIndex], true, UINT64_MAX);
                    vkResetFences(renderer.device, 1, &renderer.imageReadyToRenderFences[currentFrameIndex]); // Set fences to unsignaled state
                }
#endif

                u32 imageIndex = UINT_MAX;
                VkResult acquireNextImageResult = vkAcquireNextImageKHR(renderer.device, renderer.swapchain, 
                                                    UINT64_MAX, // TODO(joon) : Why not just use the next image, instead of waiting this image?
                                                    renderer.imageReadyToRenderSemaphores[currentFrameIndex], // signaled when the application can use the image.
                                                    0, // fence that will signaled when the application can use the image. unused for now.
                                                    &imageIndex);
                                                    
                if(acquireNextImageResult == VK_SUCCESS)
                {
                    VkCommandBuffer *commandBuffer = renderer.graphicsCommandBuffers + imageIndex;

                    uniform_buffer ubo = {};
                    ubo.model = Scale(1.0f, 1.0f, 1.0f);
                    // TODO(joon) : The coordinates that feeded into this matrix is in view space, but in world unit
                    // so to be more specific, we need some kind of unit that defines the size of the camera frustum, and use that as r(ight) & t(top) value
                    // NOTE(joon) : glm::perspective has value of 1 for both x and y value

                    r32 c = 10.0f;
                    m4 proj = SymmetricProjection(windowWidthOverHeight, 1, 0.1f, 100.0f);
                    ubo.projView = 
                        proj *
                        QuarternionRotationM4(V3(1, 0, 0), -camera.alongX)*
                        QuarternionRotationM4(V3(0, 1, 0), -camera.alongY)*
                        QuarternionRotationM4(V3(0, 0, 1), -camera.alongZ)*
                        Translate(-camera.p.x, -camera.p.y, -camera.p.z);

                    local_persist r32 angle = 0.0f;
                    r32 radius = xCount*dim/2.0f;;
                    v3 lightP = V3(radius*cosf(angle), 100.0f, radius*sinf(angle));
                    lightP.x += xCount*dim/2.0f;
                    lightP.z += zCount*dim/2.0f;
                    angle += 0.0125f;
                    ubo.lightP = lightP;

                    u64 aligned_uniform_buffer_offset = currentFrameIndex*(sizeof(uniform_buffer)/renderer.uniform_buffer_offset_alignment)*renderer.uniform_buffer_offset_alignment;
                    VkCopyMemoryToHostVisibleCoherentBuffer(&uniformBuffer, aligned_uniform_buffer_offset, &ubo, sizeof(ubo));

                    // TODO(joon) : Check this value!
                    VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                    VkSubmitInfo queueSubmitInfo = {};
                    queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                    queueSubmitInfo.waitSemaphoreCount = 1;
                    queueSubmitInfo.pWaitSemaphores = renderer.imageReadyToRenderSemaphores + currentFrameIndex;
                    queueSubmitInfo.pWaitDstStageMask = &waitFlags;
                    queueSubmitInfo.commandBufferCount = 1;
                    queueSubmitInfo.pCommandBuffers = commandBuffer;
                    queueSubmitInfo.signalSemaphoreCount = 1;
                    queueSubmitInfo.pSignalSemaphores = renderer.imageReadyToPresentSemaphores + currentFrameIndex;

                    // TODO(joon) : Do we really need to specify fence here?
                    CheckVkResult(vkQueueSubmit(renderer.graphicsQueue, 1, &queueSubmitInfo, 0));
#if 0
                    for(;;)
                    {
                        if(vkGetFenceStatus(renderer.device, renderer.imageReadyToPresentFences[currentFrameIndex]) == VK_SUCCESS) // waiting for the fence to be signaled
                        {
                            vkWaitForFences(renderer.device, 1, &renderer.imageReadyToPresentFences[currentFrameIndex], true, UINT64_MAX);
                            vkResetFences(renderer.device, 1, &renderer.imageReadyToPresentFences[currentFrameIndex]); // Set fences to unsignaled state
                            break;
                        }
                    }
#endif

                    VkPresentInfoKHR presentInfo = {};
                    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
                    presentInfo.waitSemaphoreCount = 1;
                    presentInfo.pWaitSemaphores = renderer.imageReadyToPresentSemaphores + currentFrameIndex;
                    presentInfo.swapchainCount = 1;
                    presentInfo.pSwapchains = &renderer.swapchain;
                    presentInfo.pImageIndices = &imageIndex;

                    CheckVkResult(vkQueuePresentKHR(renderer.graphicsQueue, &presentInfo));

                    // NOTE(joon) : equivalent to calling vkQueueWaitIdle for all queues
                    CheckVkResult(vkDeviceWaitIdle(renderer.device));
                }
                else if(acquireNextImageResult == VK_SUBOPTIMAL_KHR)
                {
                    // TODO(joon) : screen resized!
                }
                else if(acquireNextImageResult == VK_NOT_READY)
                {
                    // NOTE(joon) : Just use the next image
                }
                else
                {
                    Assert(acquireNextImageResult != VK_TIMEOUT);
                    // NOTE : If the code gets inside here, something really bad has happened
                    //VK_ERROR_OUT_OF_HOST_MEMORY
                    //VK_ERROR_OUT_OF_DEVICE_MEMORY
                    //VK_ERROR_DEVICE_LOST
                    //VK_ERROR_OUT_OF_DATE_KHR
                    //VK_ERROR_SURFACE_LOST_KHR
                    //VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT
                }

                currentFrameIndex = (currentFrameIndex + 1)%renderer.swapchainImageCount;
            }

            CleanVulkan(&renderer);
        }
    }

    return 0;
}
