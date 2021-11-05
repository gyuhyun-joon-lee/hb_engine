/*
 * Written by Gyuhyun 'Joon' Lee
 * https://github.com/meka-lopo/
 */
#include <windows.h>

#define VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_MESH_SHADER

#include "meka_platform_independent.h"
#include "meka_vulkan_function_loader.h"
#include "meka_mesh_loader.cpp"
#include "meka_vulkan.cpp"

global_variable b32 isEngineRunning;
global_variable b32 isWDown;
global_variable b32 isADown;
global_variable b32 isSDown;
global_variable b32 isDDown;

internal
WRITE_CONSOLE(Win32WriteConsole)
{
    u32 numberOfCharsToWrite = GetCharCountNullTerminated(buffer);
    Assert(numberOfCharsToWrite > 0);
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

struct camera
{
    v3 p;
    union
    {
        struct
        {
            r32 alongX;
            r32 alongY;
            r32 alongZ;
        };

        struct
        {
            r32 roll; 
            r32 pitch; 
            r32 yaw;  
        };

        r32 e[3];
    };
};

struct uniform_buffer
{
    // NOTE(joon) : mat4 should be 16 bytes aligned
    m4 model;
    m4 projView;
};

int CALLBACK 
WinMain(HINSTANCE instanceHandle,
		HINSTANCE prevInstanceHandle, 
		LPSTR cmd, 
		int cmdShowMethod)
{


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

            globalWriteConsoleInfo.console = Win32CreateConsoleWindow();
            globalWriteConsoleInfo.WriteConsole = Win32WriteConsole;

            Assert(globalWriteConsoleInfo.console && globalWriteConsoleInfo.WriteConsole);

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

            //load_mesh_file_result mesh = ReadSingleMeshOnlygltf(&platformApi, "../textures/cube.gltf", "../textures/cube.bin");
            //load_mesh_file_result mesh = ReadSingleMeshOnlygltf(&platformApi, "../textures/BarramundiFish/BarramundiFish.gltf", "../textures/BarramundiFish/BarramundiFish.bin");
            load_mesh_file_result mesh = ReadSingleMeshOnlygltf(&platformApi, "../textures/damaged_helmet/DamagedHelmet.gltf", "../textures/damaged_helmet/DamagedHelmet.bin");
            //OptimizeMeshGH(&mesh, 0.5f);

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
            Print("vulkan initialized");
            // IMPORTANT(joon): no rotation means camera's orientation is the same as the world coordinate
            // which means the forward vector is 0, 0, -1
            camera camera = {};
            camera.p = {0, 0, 1};

            vk_host_visible_coherent_buffer vertexBuffer = VkCreateHostVisibleCoherentBuffer(renderer.physicalDevice, renderer.device, 
                                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            sizeof(v3)*mesh.vertexCount);
            VkCopyMemoryToHostVisibleCoherentBuffer(&vertexBuffer, 0, mesh.vertices, vertexBuffer.size);

            vk_host_visible_coherent_buffer indexBuffer = VkCreateHostVisibleCoherentBuffer(renderer.physicalDevice, renderer.device, 
                                                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            sizeof(u16)*mesh.indexCount);
            VkCopyMemoryToHostVisibleCoherentBuffer(&indexBuffer, 0, mesh.indices, indexBuffer.size);

            uniform_buffer *uniformBuffers = PushArray(&platformMemory, uniform_buffer, renderer.swapchainImageCount);
            uniformBuffers[0].model = Scale(100.0f, 100.0f, 100.0f);
            uniformBuffers[1].model = uniformBuffers[0].model;
            uniformBuffers[2].model = uniformBuffers[0].model;

            // NOTE(joon) : view matrix is bascially a matrix that transforms certain position when the camera is looking at (0, 0, -1)
            uniformBuffers[1].projView = uniformBuffers[0].projView;
            uniformBuffers[2].projView = uniformBuffers[0].projView;

            vk_host_visible_coherent_buffer uniformBuffer = VkCreateHostVisibleCoherentBuffer(renderer.physicalDevice, renderer.device, 
                                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                            sizeof(uniform_buffer)*renderer.swapchainImageCount);
            VkCopyMemoryToHostVisibleCoherentBuffer(&uniformBuffer, 0, uniformBuffers, uniformBuffer.size);

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
                clearValues[0].color = {1.0f, 0.01f, 0.01f, 1};
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

                VkDescriptorBufferInfo bufferInfo = {};
                bufferInfo.buffer = vertexBuffer.buffer;
                bufferInfo.offset = 0;
                bufferInfo.range = vertexBuffer.size;

                VkWriteDescriptorSet writeDescriptorSet[2] = {};
                writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSet[0].dstSet = 0;
                writeDescriptorSet[0].dstBinding = 0;
                writeDescriptorSet[0].descriptorCount = 1;
                writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writeDescriptorSet[0].pBufferInfo = &bufferInfo;

                VkDescriptorBufferInfo uniformBufferInfo = {};
                uniformBufferInfo.buffer = uniformBuffer.buffer;
                uniformBufferInfo.offset = swapchainImageIndex*sizeof(uniform_buffer);
                uniformBufferInfo.range = sizeof(uniform_buffer);

                writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                writeDescriptorSet[1].dstSet = 0;
                writeDescriptorSet[1].dstBinding = 1;
                writeDescriptorSet[1].descriptorCount = 1;
                writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writeDescriptorSet[1].pBufferInfo = &uniformBufferInfo;

                vkCmdPushDescriptorSetKHR(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                        renderer.pipelineLayout, 0, ArrayCount(writeDescriptorSet), writeDescriptorSet);

                //VkDeviceSize offset = 0; 
                //vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);
                vkCmdBindIndexBuffer(*commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
                vkCmdDrawIndexed(*commandBuffer, mesh.indexCount, 1, 0, 0, 0);

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
                if(GetWindowRect(windowHandle, &windowRect))
                {
                }

                POINT currentMouseP = {};
                if(GetCursorPos(&currentMouseP))
                {
                    mouseDx = 0;
                    mouseDy = 0;

                    if(currentMouseP.x >= windowRect.left && currentMouseP.y >= windowRect.top &&
                       currentMouseP.x < windowRect.right && currentMouseP.y < windowRect.bottom)
                    {
                        mouseDx = (i32)currentMouseP.x-mousex;
                        mouseDy = (i32)currentMouseP.y-mousey;
                    }

                    //printf("%i, %i\n", currentMouseP.x, currentMouseP.y);

                    mousex = (i32)currentMouseP.x;
                    mousey = (i32)currentMouseP.y;
                }
                
                camera.alongY += -4.0f*(mouseDx/(r32)windowWidth);
                camera.alongX += -4.0f*(mouseDy/(r32)windowHeight);

                v3 aasdf = QuarternionRotationV100(1.5f, V3(0, 0, -1));

                v3 cameraDir = Normalize(QuarternionRotationV001(camera.alongZ, 
                                        QuarternionRotationV010(camera.alongY, 
                                         QuarternionRotationV100(camera.alongX, V3(0, 0, -1)))));
                // NOTE(joon) : For a rotation matrix, order is x->y->z 
                // which means the matrices should be ordered in z*y*x*vector
                // However, for the view matrix, because the rotation should be inversed,
                // it should be ordered in x*y*z!
                if(isWDown)
                {
                    camera.p += 0.1f*cameraDir;
                }
                
                if(isSDown)
                {
                    camera.p -= 0.1f*cameraDir;
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

                    uniformBuffers[currentFrameIndex].model = Scale(1.0f, 1.0f, 1.0f);
                    m4 proj = SymmetricProjection(0.5f*windowWidthOverHeight, 0.5f, 1.0f, 100.0f);
                    uniformBuffers[currentFrameIndex].projView = proj*
                                                                QuarternionRotationM4(V3(1, 0, 0), -camera.alongX)*
                                                                QuarternionRotationM4(V3(0, 1, 0), -camera.alongY)*
                                                                QuarternionRotationM4(V3(0, 0, 1), -camera.alongZ)*
                                                                Translate(-camera.p.x, -camera.p.y, -camera.p.z);

                    VkCopyMemoryToHostVisibleCoherentBuffer(&uniformBuffer, sizeof(uniformBuffer)*currentFrameIndex, uniformBuffers, uniformBuffer.size);

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
        }
    }

    return 0;
}
