#ifndef MIMIC_VULKAN_FUNCTION_POINTERS
#define MIMIC_VULKAN_FUNCTION_POINTERS

// NOTE(joon) : These disables pre defined functions
#define VK_NO_PROTOTYPES 
#include <vulkan/vulkan.h>

#define VFType(name) PFN_##name
// NOTE(joon) : global, instance, device level proc address loader functions
global_variable VFType(vkGetInstanceProcAddr) vkGetInstanceProcAddr;

// NOTE(joon) : Pre-intance level functions
global_variable VFType(vkCreateInstance) vkCreateInstance;
global_variable VFType(vkEnumerateInstanceLayerProperties) vkEnumerateInstanceLayerProperties;
global_variable VFType(vkEnumerateInstanceExtensionProperties) vkEnumerateInstanceExtensionProperties;

// NOTE(joon) : Intance level functions
global_variable VFType(vkEnumeratePhysicalDevices) vkEnumeratePhysicalDevices;
global_variable VFType(vkGetPhysicalDeviceProperties) vkGetPhysicalDeviceProperties;
global_variable VFType(vkGetPhysicalDeviceFeatures) vkGetPhysicalDeviceFeatures;
global_variable VFType(vkGetPhysicalDeviceQueueFamilyProperties) vkGetPhysicalDeviceQueueFamilyProperties;
global_variable VFType(vkCreateDevice) vkCreateDevice;
global_variable VFType(vkGetDeviceProcAddr) vkGetDeviceProcAddr;
global_variable VFType(vkDestroyInstance) vkDestroyInstance;
global_variable VFType(vkGetPhysicalDeviceSurfaceSupportKHR) vkGetPhysicalDeviceSurfaceSupportKHR;
global_variable VFType(vkCreateDebugUtilsMessengerEXT) vkCreateDebugUtilsMessengerEXT;
global_variable VFType(vkGetPhysicalDeviceSurfaceFormatsKHR) vkGetPhysicalDeviceSurfaceFormatsKHR;
global_variable VFType(vkGetPhysicalDeviceSurfacePresentModesKHR) vkGetPhysicalDeviceSurfacePresentModesKHR;
global_variable VFType(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
global_variable VFType(vkGetPhysicalDeviceMemoryProperties) vkGetPhysicalDeviceMemoryProperties;

// NOTE(joon) : Device level functions
#ifdef VK_USE_PLATFORM_WIN32_KHR
global_variable VFType(vkCreateWin32SurfaceKHR) vkCreateWin32SurfaceKHR;
#endif
global_variable VFType(vkCreateSwapchainKHR) vkCreateSwapchainKHR;
global_variable VFType(vkDestroySwapchainKHR) vkDestroySwapchainKHR;
global_variable VFType(vkGetSwapchainImagesKHR) vkGetSwapchainImagesKHR;
global_variable VFType(vkAcquireNextImageKHR) vkAcquireNextImageKHR;
global_variable VFType(vkQueuePresentKHR) vkQueuePresentKHR;
global_variable VFType(vkGetDeviceQueue) vkGetDeviceQueue;
global_variable VFType(vkCreateCommandPool) vkCreateCommandPool;
global_variable VFType(vkCreateRenderPass) vkCreateRenderPass;
global_variable VFType(vkCreateShaderModule) vkCreateShaderModule;
global_variable VFType(vkCreatePipelineLayout) vkCreatePipelineLayout;
global_variable VFType(vkCreateGraphicsPipelines) vkCreateGraphicsPipelines;
global_variable VFType(vkDestroyShaderModule) vkDestroyShaderModule;
global_variable VFType(vkCreateImageView) vkCreateImageView;
global_variable VFType(vkCreateFramebuffer) vkCreateFramebuffer;
global_variable VFType(vkAllocateCommandBuffers) vkAllocateCommandBuffers;
global_variable VFType(vkCreateSemaphore) vkCreateSemaphore;
global_variable VFType(vkCreateFence) vkCreateFence;
global_variable VFType(vkCmdBeginRenderPass) vkCmdBeginRenderPass;
global_variable VFType(vkCmdEndRenderPass) vkCmdEndRenderPass;
global_variable VFType(vkCreateBuffer) vkCreateBuffer;
global_variable VFType(vkAllocateMemory) vkAllocateMemory;
global_variable VFType(vkCmdBindVertexBuffers) vkCmdBindVertexBuffers;
global_variable VFType(vkBindBufferMemory) vkBindBufferMemory;
global_variable VFType(vkBeginCommandBuffer) vkBeginCommandBuffer;
global_variable VFType(vkEndCommandBuffer) vkEndCommandBuffer;
global_variable VFType(vkCmdUpdateBuffer) vkCmdUpdateBuffer;
global_variable VFType(vkGetBufferMemoryRequirements) vkGetBufferMemoryRequirements;
global_variable VFType(vkCmdBindPipeline) vkCmdBindPipeline;
global_variable VFType(vkCmdDraw) vkCmdDraw;
global_variable VFType(vkQueueSubmit) vkQueueSubmit;
global_variable VFType(vkCmdClearColorImage) vkCmdClearColorImage;
global_variable VFType(vkCmdPipelineBarrier) vkCmdPipelineBarrier;
global_variable VFType(vkQueueWaitIdle) vkQueueWaitIdle;
global_variable VFType(vkGetFenceStatus) vkGetFenceStatus;
global_variable VFType(vkWaitForFences) vkWaitForFences;
global_variable VFType(vkResetFences) vkResetFences;
global_variable VFType(vkDeviceWaitIdle) vkDeviceWaitIdle;
global_variable VFType(vkCmdSetViewport) vkCmdSetViewport;
global_variable VFType(vkCmdSetScissor) vkCmdSetScissor;

#define GetInstanceFunction(instance, name) name = (VFType(name))vkGetInstanceProcAddr(instance, #name); Assert(name)
#define GetDeviceFunction(device, name) name = (VFType(name))vkGetDeviceProcAddr(device, #name); Assert(name)

// TODO(joon) : Make this more platform-independent!
#ifdef VK_USE_PLATFORM_WIN32_KHR
#define LoadProcAddress(function) function = (VFType(function))GetProcAddress(library, #function)

internal void
Win32LoadVulkanLibrary(char *filename)
{
    HMODULE library = LoadLibraryA(filename);
    if(library)
    {
        LoadProcAddress(vkGetInstanceProcAddr);
    }
    else
    {
        // TODO(joon) : Log
        Assert(0);
    }
}

#elif VK_USE_PLATFORM_MACOS_MVK // TODO(joon) : VK_USE_PLATFORM_METAL_EXT?
#endif

internal void
ResolvePreInstanceLevelFunctions()
{
    GetInstanceFunction(0, vkCreateInstance);
    GetInstanceFunction(0, vkEnumerateInstanceLayerProperties);
    GetInstanceFunction(0, vkEnumerateInstanceExtensionProperties);
}

internal void
ResolveInstanceLevelFunctions(VkInstance instance)
{
    GetInstanceFunction(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    GetInstanceFunction(instance, vkEnumeratePhysicalDevices);
    GetInstanceFunction(instance, vkGetPhysicalDeviceProperties);
    GetInstanceFunction(instance, vkGetDeviceProcAddr);
    GetInstanceFunction(instance, vkGetPhysicalDeviceFeatures);
    GetInstanceFunction(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    GetInstanceFunction(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
    GetInstanceFunction(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);

    GetInstanceFunction(instance, vkCreateDevice);
    GetInstanceFunction(instance, vkDestroyInstance);
    GetInstanceFunction(instance, vkGetPhysicalDeviceSurfaceSupportKHR);
    GetInstanceFunction(instance, vkCreateDebugUtilsMessengerEXT);
    GetInstanceFunction(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    GetInstanceFunction(instance, vkGetPhysicalDeviceMemoryProperties);

#ifdef VK_USE_PLATFORM_WIN32_KHR
    GetInstanceFunction(instance, vkCreateWin32SurfaceKHR);
#endif
}

internal void
ResolveDeviceLevelFunctions(VkDevice device)
{
    GetDeviceFunction(device, vkCreateSwapchainKHR);
    GetDeviceFunction(device, vkDestroySwapchainKHR);
    GetDeviceFunction(device, vkGetSwapchainImagesKHR);
    GetDeviceFunction(device, vkAcquireNextImageKHR);
    GetDeviceFunction(device, vkQueuePresentKHR);
    GetDeviceFunction(device, vkGetDeviceQueue);
    GetDeviceFunction(device, vkCreateCommandPool);
    GetDeviceFunction(device, vkCreateRenderPass);
    GetDeviceFunction(device, vkCreateShaderModule);
    GetDeviceFunction(device, vkCreatePipelineLayout);
    GetDeviceFunction(device, vkCreateGraphicsPipelines);
    GetDeviceFunction(device, vkDestroyShaderModule);
    GetDeviceFunction(device, vkCreateImageView);
    GetDeviceFunction(device, vkCreateFramebuffer);
    GetDeviceFunction(device, vkAllocateCommandBuffers);
    GetDeviceFunction(device, vkCreateSemaphore);
    GetDeviceFunction(device, vkCreateFence);
    GetDeviceFunction(device, vkCmdBeginRenderPass);
    GetDeviceFunction(device, vkCmdEndRenderPass);
    GetDeviceFunction(device, vkCreateBuffer);
    GetDeviceFunction(device, vkAllocateMemory);
    GetDeviceFunction(device, vkCmdBindVertexBuffers);
    GetDeviceFunction(device, vkBindBufferMemory);
    GetDeviceFunction(device, vkBeginCommandBuffer);
    GetDeviceFunction(device, vkEndCommandBuffer);
    GetDeviceFunction(device, vkCmdUpdateBuffer);
    GetDeviceFunction(device, vkGetBufferMemoryRequirements);
    GetDeviceFunction(device, vkCmdDraw);
    GetDeviceFunction(device, vkCmdBindPipeline);
    GetDeviceFunction(device, vkQueueSubmit);
    GetDeviceFunction(device, vkCmdClearColorImage);
    GetDeviceFunction(device, vkCmdPipelineBarrier);
    GetDeviceFunction(device, vkQueueWaitIdle);
    GetDeviceFunction(device, vkGetFenceStatus);
    GetDeviceFunction(device, vkWaitForFences);
    GetDeviceFunction(device, vkResetFences);
    GetDeviceFunction(device, vkDeviceWaitIdle);
    GetDeviceFunction(device, vkCmdSetViewport);
    GetDeviceFunction(device, vkCmdSetScissor);
        
        
}

#endif
