#ifndef MEKA_VULKAN_FUNCTION_POINTERS
#define MEKA_VULKAN_FUNCTION_POINTERS

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
global_variable VFType(vkEnumerateDeviceExtensionProperties) vkEnumerateDeviceExtensionProperties;
global_variable VFType(vkDestroySurfaceKHR) vkDestroySurfaceKHR;

// NOTE(joon) : Device level functions
#ifdef MEKA_WIN32
global_variable VFType(vkCreateWin32SurfaceKHR) vkCreateWin32SurfaceKHR;
#elif MEKA_MACOS
global_variable VFType(vkCreateMetalSurfaceEXT) vkCreateMetalSurfaceEXT;
#endif

global_variable VFType(vkCreateSwapchainKHR) vkCreateSwapchainKHR;
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
global_variable VFType(vkMapMemory) vkMapMemory;
global_variable VFType(vkCmdBindIndexBuffer) vkCmdBindIndexBuffer;
global_variable VFType(vkCmdDrawIndexed) vkCmdDrawIndexed;
global_variable VFType(vkCreateDescriptorSetLayout) vkCreateDescriptorSetLayout;
global_variable VFType(vkCmdPushDescriptorSetKHR) vkCmdPushDescriptorSetKHR;
global_variable VFType(vkCreateImage) vkCreateImage;
global_variable VFType(vkGetImageMemoryRequirements) vkGetImageMemoryRequirements;
global_variable VFType(vkBindImageMemory) vkBindImageMemory;
global_variable VFType(vkDestroyPipelineLayout) vkDestroyPipelineLayout;
global_variable VFType(vkDestroySemaphore) vkDestroySemaphore;
global_variable VFType(vkDestroyFence) vkDestroyFence;
global_variable VFType(vkFreeCommandBuffers) vkFreeCommandBuffers;
global_variable VFType(vkDestroyCommandPool) vkDestroyCommandPool;
global_variable VFType(vkDestroyFramebuffer) vkDestroyFramebuffer;
global_variable VFType(vkFreeMemory) vkFreeMemory;
global_variable VFType(vkDestroyImageView) vkDestroyImageView;
global_variable VFType(vkDestroyImage) vkDestroyImage;
global_variable VFType(vkDestroyPipeline) vkDestroyPipeline;
global_variable VFType(vkDestroyRenderPass) vkDestroyRenderPass;
global_variable VFType(vkDestroySwapchainKHR) vkDestroySwapchainKHR;
global_variable VFType(vkDestroyDevice) vkDestroyDevice;

#define get_instance_function(instance, name) name = (VFType(name))vkGetInstanceProcAddr(instance, #name); assert(name)
#define get_device_function(device, name) name = (VFType(name))vkGetDeviceProcAddr(device, #name); assert(name)

// TODO(joon) : Make this more platform-independent!
#ifdef MEKA_WIN32

internal void
Win32LoadVulkanLibrary(char *filename)
{
    HMODULE library = LoadLibraryA(filename);
    if(library)
    {
        vkGetInstanceProcAddr = (VFType(vkGetInstanceProcAddr))GetProcAddress(library, vkGetInstanceProcAddr);
    }
    else
    {
        // TODO(joon) : Log
        assert(0);
    }
}

#elif VK_MACOS // TODO(joon) : VK_USE_PLATFORM_METAL_EXT?
#endif

internal void
resolve_pre_instance_functions()
{
    get_instance_function(0, vkCreateInstance);
    get_instance_function(0, vkEnumerateInstanceLayerProperties);
    get_instance_function(0, vkEnumerateInstanceExtensionProperties);
}

internal void
resolve_instance_functions(VkInstance instance)
{
    get_instance_function(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    get_instance_function(instance, vkEnumeratePhysicalDevices);
    get_instance_function(instance, vkGetPhysicalDeviceProperties);
    get_instance_function(instance, vkGetDeviceProcAddr);
    get_instance_function(instance, vkGetPhysicalDeviceFeatures);
    get_instance_function(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    get_instance_function(instance, vkGetPhysicalDeviceSurfaceFormatsKHR);
    get_instance_function(instance, vkGetPhysicalDeviceSurfacePresentModesKHR);

    get_instance_function(instance, vkCreateDevice);
    get_instance_function(instance, vkDestroyInstance);
    get_instance_function(instance, vkGetPhysicalDeviceSurfaceSupportKHR);
    get_instance_function(instance, vkCreateDebugUtilsMessengerEXT);
    get_instance_function(instance, vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    get_instance_function(instance, vkGetPhysicalDeviceMemoryProperties);
    get_instance_function(instance, vkEnumerateDeviceExtensionProperties);
    get_instance_function(instance, vkDestroySurfaceKHR);

#ifdef MEKA_WIN32
    get_instance_function(instance, vkCreateWin32SurfaceKHR);
#elif MEKA_MACOS
    get_instance_function(instance, vkCreateMetalSurfaceEXT);
#endif
}

internal void
ResolveDeviceLevelFunctions(VkDevice device)
{
    get_device_function(device, vkCreateSwapchainKHR);
    get_device_function(device, vkDestroySwapchainKHR);
    get_device_function(device, vkGetSwapchainImagesKHR);
    get_device_function(device, vkAcquireNextImageKHR);
    get_device_function(device, vkQueuePresentKHR);
    get_device_function(device, vkGetDeviceQueue);
    get_device_function(device, vkCreateCommandPool);
    get_device_function(device, vkCreateRenderPass);
    get_device_function(device, vkCreateShaderModule);
    get_device_function(device, vkCreatePipelineLayout);
    get_device_function(device, vkCreateGraphicsPipelines);
    get_device_function(device, vkDestroyShaderModule);
    get_device_function(device, vkCreateImageView);
    get_device_function(device, vkCreateFramebuffer);
    get_device_function(device, vkAllocateCommandBuffers);
    get_device_function(device, vkCreateSemaphore);
    get_device_function(device, vkCreateFence);
    get_device_function(device, vkCmdBeginRenderPass);
    get_device_function(device, vkCmdEndRenderPass);
    get_device_function(device, vkCreateBuffer);
    get_device_function(device, vkAllocateMemory);
    get_device_function(device, vkCmdBindVertexBuffers);
    get_device_function(device, vkBindBufferMemory);
    get_device_function(device, vkBeginCommandBuffer);
    get_device_function(device, vkEndCommandBuffer);
    get_device_function(device, vkCmdUpdateBuffer);
    get_device_function(device, vkGetBufferMemoryRequirements);
    get_device_function(device, vkCmdDraw);
    get_device_function(device, vkCmdBindPipeline);
    get_device_function(device, vkQueueSubmit);
    get_device_function(device, vkCmdClearColorImage);
    get_device_function(device, vkCmdPipelineBarrier);
    get_device_function(device, vkQueueWaitIdle);
    get_device_function(device, vkGetFenceStatus);
    get_device_function(device, vkWaitForFences);
    get_device_function(device, vkResetFences);
    get_device_function(device, vkDeviceWaitIdle);
    get_device_function(device, vkCmdSetViewport);
    get_device_function(device, vkCmdSetScissor);
    get_device_function(device, vkMapMemory);
    get_device_function(device, vkCmdBindIndexBuffer);
    get_device_function(device, vkCmdDrawIndexed);
    get_device_function(device, vkCreateDescriptorSetLayout);
    get_device_function(device, vkCmdPushDescriptorSetKHR); // Extension
    get_device_function(device, vkCreateImage);
    get_device_function(device, vkGetImageMemoryRequirements);
    get_device_function(device, vkBindImageMemory);
    get_device_function(device, vkDestroyPipelineLayout);
    get_device_function(device, vkDestroySemaphore);
    get_device_function(device, vkDestroyFence);
    get_device_function(device, vkFreeCommandBuffers);
    get_device_function(device, vkDestroyCommandPool);
    get_device_function(device, vkDestroyFramebuffer);
    get_device_function(device, vkFreeMemory);
    get_device_function(device, vkDestroyImageView);
    get_device_function(device, vkDestroyImage);
    get_device_function(device, vkDestroyPipelineLayout);
    get_device_function(device, vkDestroyPipeline);
    get_device_function(device, vkDestroyRenderPass);
    get_device_function(device, vkDestroySwapchainKHR);
    get_device_function(device, vkDestroyDevice);
   // get_device_function(device, );

}

#endif
