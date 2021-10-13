#ifndef MIMIC_VULKAN_FUNCTION_POINTERS
#define MIMIC_VULKAN_FUNCTION_POINTERS

// NOTE(joon) : These disables pre defined functions
#define VK_NO_PROTOTYPES 
#include <vulkan/vulkan.h>

#define VFType(name) PFN_##name
// NOTE(joon) : global, instance, device level proc address loader functions
global_variable VFType(vkGetInstanceProcAddr) vkGetInstanceProcAddr;

global_variable VFType(vkGetPhysicalDeviceQueueFamilyProperties) vkGetPhysicalDeviceQueueFamilyProperties;
global_variable VFType(vkGetDeviceProcAddr) vkGetDeviceProcAddr;
global_variable VFType(vkCreateInstance) vkCreateInstance;
global_variable VFType(vkEnumeratePhysicalDevices) vkEnumeratePhysicalDevices;
global_variable VFType(vkEnumerateInstanceLayerProperties) vkEnumerateInstanceLayerProperties;
global_variable VFType(vkEnumerateInstanceExtensionProperties) vkEnumerateInstanceExtensionProperties;

global_variable VFType(vkCreateDevice) vkCreateDevice;
global_variable VFType(vkCreateWin32SurfaceKHR) vkCreateWin32SurfaceKHR;
global_variable VFType(vkCreateSwapchainKHR) vkCreateSwapchainKHR;

global_variable VFType(vkGetPhysicalDeviceProperties) vkGetPhysicalDeviceProperties;

#define GetInstanceFunction(instance, name) name = (VFType(name))vkGetInstanceProcAddr(instance, #name); Assert(name)
#define GetDeviceFunction(device, name) name = (VFType(name))vkGetDeviceProcAddr(device, #name); Assert(name)

// TODO(joon) : Make this more platform-independent!
#ifdef VK_USE_PLATFORM_WIN32_KHR
#define LoadProcAddress(function) function = (VFType(function))GetProcAddress(handle, #function)

internal void
Win32LoadVulkanLibrary(char *filename)
{
    HMODULE handle = LoadLibraryA(filename);
    if(handle)
    {
        LoadProcAddress(vkGetInstanceProcAddr);
        LoadProcAddress(vkGetDeviceProcAddr);
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
ResolveInstanceLevelFunctions(VkInstance instance)
{
    GetInstanceFunction(instance, vkGetPhysicalDeviceQueueFamilyProperties);
    GetInstanceFunction(instance, vkEnumeratePhysicalDevices);
    GetInstanceFunction(instance, vkGetPhysicalDeviceProperties);
    GetInstanceFunction(instance, vkGetDeviceProcAddr);
    GetInstanceFunction(instance, vkEnumerateInstanceLayerProperties);
    GetInstanceFunction(instance, vkEnumerateInstanceExtensionProperties);

    //VK_INSTANCE_LEVEL_FUNCTION( vkDestroyInstance )
    //VK_INSTANCE_LEVEL_FUNCTION( vkEnumeratePhysicalDevices )
    //VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceProperties )
    //VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceFeatures )
    //VK_INSTANCE_LEVEL_FUNCTION( vkGetPhysicalDeviceQueueFamilyProperties )
    //VK_INSTANCE_LEVEL_FUNCTION( vkGetDeviceProcAddr )
    //VK_INSTANCE_LEVEL_FUNCTION( vkEnumerateDeviceExtensionProperties )
}

internal void
ResolveDeviceLevelFunctions(VkDevice device)
{
    GetDeviceFunction(device, vkCreateWin32SurfaceKHR);
    GetDeviceFunction(device, vkCreateSwapchainKHR);
}

#endif
