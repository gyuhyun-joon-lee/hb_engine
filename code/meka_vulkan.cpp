/*j
 * Written by Joon 'Mimic' Lee
 * Full source code in https://github.com/joon-mimic-lee/mimic_renderer
 */

#include <stdio.h>
#define CheckVkResult(expression)                                             \
{                                                                             \
    VkResult vkResult = (expression);                                           \
    if(vkResult != VK_SUCCESS)                                                  \
    {                                                                           \
        printf("%s failed with VkResult, %u\n", #expression, vkResult);         \
        Assert(0);                                                              \
    }                                                                           \
}

internal VkPhysicalDevice
FindPhysicalDevice(VkInstance instance)
{
    VkPhysicalDevice result = {};
    u32 physicalDeviceCount = 0;
    CheckVkResult(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, 0));
    // NOTE(joon) : We would normally query the physical device count first and then
    // allocate a buffer based on the count to get all the physical devices,
    // but this works fine(for our needs)
    VkPhysicalDevice physicalDevices[16];
    CheckVkResult(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));

    for(u32 physicalDeviceIndex = 0;
        physicalDeviceIndex < physicalDeviceCount;
        ++physicalDeviceIndex)
    {
        VkPhysicalDevice *physicalDevice = physicalDevices + physicalDeviceIndex;
        VkPhysicalDeviceProperties property;
        vkGetPhysicalDeviceProperties(*physicalDevice, &property);

        if(property.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ||
            property.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            result = *physicalDevice;
            break;
        }
    }

    return result;
}

internal VkInstance
CreateInstance()
{
    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, 0);
    VkLayerProperties layers[64];
    vkEnumerateInstanceLayerProperties(&layerCount, layers);
    // NOTE(joon) : VK_LAYER_LUNARG_standard_validation is deprecated
    char *instanceLayerNames[] = 
    {
        "VK_LAYER_KHRONOS_validation"
    };
    
    u32 extensionCount;
    vkEnumerateInstanceLayerProperties(&extensionCount, 0);
    VkLayerProperties extensions[64];
    vkEnumerateInstanceLayerProperties(&extensionCount, extensions);
    char *instanceLevelExtensions[] =
    {
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME, // TODO(joon) : Find out what this does?
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface"
    };

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = 0;
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.enabledLayerCount = ArrayCount(instanceLayerNames);
    instanceCreateInfo.ppEnabledLayerNames = instanceLayerNames;
    instanceCreateInfo.enabledExtensionCount = ArrayCount(instanceLevelExtensions);
    instanceCreateInfo.ppEnabledExtensionNames = instanceLevelExtensions;

    VkInstance instance;
    vkCreateInstance(&instanceCreateInfo, 0, &instance);

    return instance;
}

struct vk_queue_families
{
    u32 graphicsQueueFamilyIndex;
    u32 presentQueueFamilyIndex;
    u32 transferQueueFamilyIndex;
};

internal vk_queue_families
GetSupportedQueueFamilies(VkPhysicalDevice physicalDevice)
{
    vk_queue_families result = {UINT_MAX, UINT_MAX, UINT_MAX};

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, 0);
    VkQueueFamilyProperties queueFamilyProperties[16]; // HACK : The size of this array should not be hard-coded
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties);

    for(u32 queueFamilyIndex = 0;
        queueFamilyIndex < queueFamilyCount;
        ++queueFamilyIndex)
    {
        VkQueueFamilyProperties *property = queueFamilyProperties + queueFamilyIndex;

        if(property->queueFlags | VK_QUEUE_GRAPHICS_BIT)
        {
            result.graphicsQueueFamilyIndex = queueFamilyIndex;
        }

        // TODO(joon) : seperating transfer queue from graphics queue might lead to better performance
        if(property->queueFlags | VK_QUEUE_TRANSFER_BIT)
        {
            result.transferQueueFamilyIndex = queueFamilyIndex;
        }
    }

    return result;
}

internal VkDevice
CreateDevice(VkPhysicalDevice physicalDevice)
{
    GetSupportedQueueFamilies(physicalDevice);
    VkDeviceQueueCreateInfo queueCreateInfos[3];
#if 0
    VkStructureType sType;
    const void* pNext;
    VkDeviceQueueCreateFlags flags;
    uint32_t queueFamilyIndex;
    uint32_t queueCount;
    const float* pQueuePriorities;
#endif

    char *deviceExtensionNames[] =  
    {
        "VK_KHR_swapchain"
    };

    // NOTE(joon) : device layers are deprecated
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = ArrayCount(queueCreateInfos);
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.enabledExtensionCount = ArrayCount(deviceExtensionNames);
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;
    deviceCreateInfo.pEnabledFeatures = 0; // TODO(joon) : enable best practice feature!

    VkDevice device;
    vkCreateDevice(physicalDevice, &deviceCreateInfo, 0, &device);

    return device;
}

internal void
InitVulkanWin32(HINSTANCE instanceHandle, HWND windowHandle, i32 screenWidth, i32 screenHeight)
{
    GetInstanceFunction(0, vkCreateInstance);
    
    VkInstance instance = CreateInstance();
    ResolveInstanceLevelFunctions(instance);

    VkPhysicalDevice physicalDevice = FindPhysicalDevice(instance);

    VkDevice device = CreateDevice(physicalDevice);
    ResolveDeviceLevelFunctions(device);

    VkSurfaceKHR surface;

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = instanceHandle;
    surfaceCreateInfo.hwnd = windowHandle;

    vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, 0, &surface);

    VkSwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    //swapchainCreateInfo.flags; // TODO(joon) : might be worth looking into protected image?
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = 1; // TODO(joon) : If vulkan cannot allocate this many images, this will fail
    swapchainCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageExtent.width = screenWidth;
    swapchainCreateInfo.imageExtent.height = screenHeight;
    swapchainCreateInfo.imageArrayLayers = 1; // non-stereoscopic
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //swapchainCreateInfo.queueFamilyIndexCount = ;
    //swapchainCreateInfo.pQueueFamilyIndices = ;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;

    VkSwapchainKHR swapchain;
    vkCreateSwapchainKHR(device, &swapchainCreateInfo, 0, &swapchain);
}
















