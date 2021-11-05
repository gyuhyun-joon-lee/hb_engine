/*
 * Written by Gyuhyun 'Joon' Lee
 * https://github.com/meka-lopo/
 */
#define CheckVkResult(expression)                                             \
{                                                                             \
    VkResult vkResult = (expression);                                           \
    if(vkResult != VK_SUCCESS)                                                  \
    {                                                                           \
        Print("CheckVkResult Failed\n");                               \
        Assert(0);                                                              \
    }                                                                           \
}

struct vertex
{
    r32 x;
    r32 y;
    r32 z;
};

struct vk_queue_families
{
    u32 graphicsQueueFamilyIndex;
    u32 graphicsQueueCount;
    u32 presentQueueFamilyIndex;
    u32 presentQueueCount;
    u32 transferQueueFamilyIndex;
    u32 transferQueueCount;
};

// TODO(joon) : Make this graphics api independent
struct renderer
{
    // TODO(joon) : Make my own allocator so that we don't need to store the things that are not 
    // needed for the actual rendering
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;

    VkSurfaceKHR surface;
    v2 surfaceExtent;

    vk_queue_families queueFamilies;
    VkQueue graphicsQueue;
    VkQueue transferQueue;
    VkQueue presentQueue;

    // TODO(joon) : don't individually allocate memeories
    VkSwapchainKHR swapchain;
    VkImage *swapchainImages;
    u32 swapchainImageCount;
    VkImageView *swapchainImageViews; // count = swap chain image count 

    VkRenderPass renderPass;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;

    VkImage depthImage;
    VkImageView depthImageView;
    VkDeviceMemory depthImageMemory;

    VkFramebuffer *framebuffers;  // count = swap chain image count 
    VkCommandPool graphicsCommandPool;
    VkCommandBuffer *graphicsCommandBuffers; // count = swap chain image count 

    VkSemaphore *imageReadyToRenderSemaphores;// count = swap chain image count 
    VkSemaphore *imageReadyToPresentSemaphores;// count = swap chain image count 

    VkFence *imageReadyToRenderFences;// count = swap chain image count 
    VkFence *imageReadyToPresentFences;// count = swap chain image count 
};



internal VkPhysicalDevice
FindPhysicalDevice(VkInstance instance)
{
    VkPhysicalDevice result = {};
    u32 physicalDeviceCount = 0;
    CheckVkResult(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, 0));
    VkPhysicalDevice physicalDevices[16]; // HACK
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



internal VKAPI_ATTR VkBool32 VKAPI_CALL 
DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, 
                        VkDebugUtilsMessageTypeFlagsEXT types,
                        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                        void* userData)
{
    Print(callbackData->pMessage);

    return true;
}

internal void
CreateDebugMessenger(VkInstance instance)
{
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
    
    debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ;
    debugMessengerInfo.messageType = //VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerInfo.pfnUserCallback = DebugMessengerCallback;
    debugMessengerInfo.pUserData = 0;

    VkDebugUtilsMessengerEXT messenger;
    CheckVkResult(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, 0, &messenger));
}


internal vk_queue_families
GetSupportedQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    vk_queue_families result = {};
    result.graphicsQueueFamilyIndex = UINT_MAX;
    result.presentQueueFamilyIndex = UINT_MAX;
    result.transferQueueFamilyIndex = UINT_MAX;

    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, 0);
    VkQueueFamilyProperties queueFamilyProperties[16]; // HACK : The size of this array should not be hard-coded
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties);

    for(u32 queueFamilyIndex = 0;
        queueFamilyIndex < queueFamilyCount;
        ++queueFamilyIndex)
    {
        VkQueueFamilyProperties *property = queueFamilyProperties + queueFamilyIndex;

        // TODO(joon) : For these queues, what are the best way to configure these?
        VkBool32 supportSurface;
        CheckVkResult(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &supportSurface));
        if(supportSurface &&result.presentQueueFamilyIndex == UINT_MAX)
        {
            result.presentQueueFamilyIndex = queueFamilyIndex;
            result.presentQueueCount = property->queueCount;
        }

        if(property->queueFlags | VK_QUEUE_GRAPHICS_BIT)
        {
            if(result.graphicsQueueFamilyIndex == UINT_MAX)
            {
                result.graphicsQueueFamilyIndex = queueFamilyIndex;
                result.graphicsQueueCount = property->queueCount;
            }
        }

        // TODO(joon) : seperating transfer queue from graphics queue might lead to better performance
        if(property->queueFlags | VK_QUEUE_TRANSFER_BIT)
        {
            if(result.transferQueueFamilyIndex == UINT_MAX &&
                queueFamilyIndex != result.transferQueueFamilyIndex)
            {
                result.transferQueueFamilyIndex = queueFamilyIndex;
                result.transferQueueCount = property->queueCount;
            }
        }
    }

    if(result.transferQueueFamilyIndex == UINT_MAX)
    {
        result.transferQueueFamilyIndex = result.graphicsQueueFamilyIndex;
        result.transferQueueCount = result.graphicsQueueCount;
    }

    return result;
}

internal VkDevice
CreateDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, vk_queue_families *queueFamilies)
{
    // NOTE(joon) : Max queue infos we need are three, one for each graphics/present/transfer
    // TODO(joon) : If we add compute queue later, change this too!
    VkDeviceQueueCreateInfo queueCreateInfos[3] = {};
    r32 queuePriority = 1.0f;
    queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfos[0].queueFamilyIndex = queueFamilies->graphicsQueueFamilyIndex;
    // TODO(joon) : Use multiple queues?
    queueCreateInfos[0].queueCount = 1;//queueFamilies.graphicsQueueCount;
    queueCreateInfos[0].pQueuePriorities = &queuePriority;

    u32 validQueueCreateInfoCount = 1;
    if(queueFamilies->presentQueueFamilyIndex != queueFamilies->graphicsQueueFamilyIndex)
    {
        queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[1].queueFamilyIndex = queueFamilies->presentQueueFamilyIndex;
        queueCreateInfos[1].queueCount = 1;//queueFamilies.graphicsQueueCount;
        queueCreateInfos[1].pQueuePriorities = &queuePriority;
        validQueueCreateInfoCount++;
    }
    if(queueFamilies->transferQueueFamilyIndex != queueFamilies->graphicsQueueFamilyIndex &&
        queueFamilies->transferQueueFamilyIndex != queueFamilies->presentQueueFamilyIndex)
    {
        queueCreateInfos[2].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[2].queueFamilyIndex = queueFamilies->transferQueueFamilyIndex;
        queueCreateInfos[2].queueCount = 1;//queueFamilies.graphicsQueueCount;
        queueCreateInfos[2].pQueuePriorities = &queuePriority;
        validQueueCreateInfoCount++;
    }

    char *deviceExtensionNames[] =  
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME, // TODO(joon) : This is not supported on every devices
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
        //"VK_NV_mesh_shader", // TODO(joon) : Does not work with renderdoc?
    };

    u32 propertyCount = 0;
    CheckVkResult(vkEnumerateDeviceExtensionProperties(physicalDevice, 0, &propertyCount, 0));
    VkExtensionProperties availableDeviceExtensions[256]; // HACK
    CheckVkResult(vkEnumerateDeviceExtensionProperties(physicalDevice, 0, &propertyCount, availableDeviceExtensions));

    VkPhysicalDeviceMeshShaderFeaturesNV featureMeshShader = {};
    featureMeshShader.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
    featureMeshShader.pNext = 0;
    featureMeshShader.taskShader = false; // TODO(joon) : Enable this when we need task shader!
    featureMeshShader.meshShader = true; 

    VkPhysicalDevice16BitStorageFeatures feature16BitStorage = {};
    feature16BitStorage.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
    feature16BitStorage.pNext = &featureMeshShader;
    feature16BitStorage.storageBuffer16BitAccess = true;
    feature16BitStorage.uniformAndStorageBuffer16BitAccess = false;
    feature16BitStorage.storagePushConstant16 = false; // We can push 16 bit value??
    feature16BitStorage.storageInputOutput16 = false;
    
    VkPhysicalDevice8BitStorageFeatures feature8BitStorage = {};
    feature8BitStorage.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES;
    feature8BitStorage.pNext = &feature16BitStorage;
    feature8BitStorage.storageBuffer8BitAccess = true;
    feature8BitStorage.uniformAndStorageBuffer8BitAccess = false;
    feature8BitStorage.storagePushConstant8 = false;

    VkPhysicalDeviceFeatures2 feature2 = {};
    feature2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    feature2.pNext = &feature8BitStorage;

    // NOTE(joon) : device layers are deprecated
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &feature2;
    deviceCreateInfo.queueCreateInfoCount = validQueueCreateInfoCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.enabledExtensionCount = ArrayCount(deviceExtensionNames);
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;

    VkDevice device;
    CheckVkResult(vkCreateDevice(physicalDevice, &deviceCreateInfo, 0, &device));

    return device;
}

internal VkDeviceMemory
VkBufferAllocateMemory(VkPhysicalDevice physicalDevice, VkDevice device, 
                    VkBuffer buffer, VkMemoryPropertyFlags flags)
{
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

    // TODO(joon) : Store this value!
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

    u32 memoryTypeBits = memoryRequirements.memoryTypeBits;
    u32 memoryTypeIndex = 0;
    b32 isMemoryTypeSupported = false;
    while(memoryTypeBits)
    {
        if(memoryTypeBits & 1)
        {
            if((physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & flags) == flags)
            {
                isMemoryTypeSupported = true;
                break;
            }
        }

        memoryTypeBits = memoryTypeBits >> 1;
        memoryTypeIndex++;
    }
    Assert(isMemoryTypeSupported);

    // TODO(joon) : check whether the physical device actually supports the memory type or not
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = memoryTypeIndex;

    VkDeviceMemory bufferMemory;
    CheckVkResult(vkAllocateMemory(device, &memoryAllocInfo, 0, &bufferMemory));
    CheckVkResult(vkBindBufferMemory(device, buffer, bufferMemory, 0));

    return bufferMemory;
}

struct vk_host_visible_coherent_buffer
{
    VkBuffer buffer;
    VkDeviceMemory deviceMemory;

    void *memory; // memory that we can directly write into
    u64 size;
};

internal vk_host_visible_coherent_buffer
VkCreateHostVisibleCoherentBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkBufferUsageFlags usage, u64 size)
{
    vk_host_visible_coherent_buffer result = {};
    result.memory = VK_NULL_HANDLE;
    result.size = size;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = result.size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    CheckVkResult(vkCreateBuffer(device, &bufferCreateInfo, 0, &result.buffer));
    result.deviceMemory = VkBufferAllocateMemory(physicalDevice, device, result.buffer, 
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // This pointer minus offset must be aligned to at least VkPhysicalDeviceLimits::minMemoryMapAlignment.
    CheckVkResult(vkMapMemory(device, result.deviceMemory, 0, result.size, 0, &result.memory));

    return result;
}

internal void
VkCopyMemoryToHostVisibleCoherentBuffer(vk_host_visible_coherent_buffer *buffer, u64 offset, void *source, u64 sourceSize)
{
    // TODO(joon) : maybe copying one by one is a bad idea, put it inside the queue and flush it at once?
    // In that case, the memory doesnt need to be host_visible or host_coherent
    memcpy(buffer->memory, source, sourceSize);
}

internal void
Win32InitVulkan(renderer *renderer, HINSTANCE instanceHandle, HWND windowHandle, i32 screenWidth, i32 screenHeight,
                platform_api *platformApi, platform_memory *platformMemory)
{
#if !MEKA_VULKAN
    Assert(0);
#endif

    ResolvePreInstanceLevelFunctions();

    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, 0);
    VkLayerProperties layers[64]; // HACK
    vkEnumerateInstanceLayerProperties(&layerCount, layers);
    // NOTE(joon) : VK_LAYER_LUNARG_standard_validation is deprecated
    char *instanceLayerNames[] = 
    {
#if MEKA_DEBUG
        "VK_LAYER_KHRONOS_validation"
#endif
    };

    for(u32 desiredLayerIndex = 0;
            desiredLayerIndex < ArrayCount(instanceLayerNames);
            ++desiredLayerIndex)
    {
        b32 found = false;
        for(u32 layerIndex = 0;
            layerIndex < layerCount;
            ++layerIndex)
        {
            if(strcmp(instanceLayerNames[desiredLayerIndex], layers[layerIndex].layerName))
            {
                found = true;
                //WriteConsoleInfo->writt
                break;
            }
        }

        if(!found)
        {
            Print(instanceLayerNames[desiredLayerIndex]);
            Print("not found\n");
        }
        Assert(found);
    }
    
    u32 extensionCount;
    vkEnumerateInstanceExtensionProperties(0, &extensionCount, 0);
    VkExtensionProperties extensions[64];
    vkEnumerateInstanceExtensionProperties(0, &extensionCount, extensions);
    char *instanceLevelExtensions[] =
    {
#if MEKA_DEBUG
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_WIN32_KHR
        "VK_KHR_win32_surface",
#endif
        "VK_KHR_get_physical_device_properties2",
    };
    for(u32 desiredExtensionIndex = 0;
            desiredExtensionIndex < ArrayCount(instanceLevelExtensions);
            ++desiredExtensionIndex)
    {
        b32 found = false;
        for(u32 extensionIndex = 0;
            extensionIndex < extensionCount;
            ++extensionIndex)
        {
            if(StringCompare(instanceLevelExtensions[desiredExtensionIndex], extensions[extensionIndex].extensionName))
            {
                found = true;
                break;
            }
        }

        Assert(found);
    }

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pNext = 0;
    instanceCreateInfo.flags = 0;
    instanceCreateInfo.enabledLayerCount = ArrayCount(instanceLayerNames);
    instanceCreateInfo.ppEnabledLayerNames = instanceLayerNames;
    instanceCreateInfo.enabledExtensionCount = ArrayCount(instanceLevelExtensions);
    instanceCreateInfo.ppEnabledExtensionNames = instanceLevelExtensions;

    CheckVkResult(vkCreateInstance(&instanceCreateInfo, 0, &renderer->instance));

    ResolveInstanceLevelFunctions(renderer->instance);

#if MEKA_DEBUG
    CreateDebugMessenger(renderer->instance);
#endif

    renderer->physicalDevice = FindPhysicalDevice(renderer->instance);

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = instanceHandle;
    surfaceCreateInfo.hwnd = windowHandle;
    CheckVkResult(vkCreateWin32SurfaceKHR(renderer->instance, &surfaceCreateInfo, 0, &renderer->surface));

    renderer->queueFamilies = GetSupportedQueueFamilies(renderer->physicalDevice, renderer->surface);
    renderer->device = CreateDevice(renderer->physicalDevice, renderer->surface, &renderer->queueFamilies);
    ResolveDeviceLevelFunctions(renderer->device);

    vkGetDeviceQueue(renderer->device, renderer->queueFamilies.graphicsQueueFamilyIndex, 0, &renderer->graphicsQueue);
    vkGetDeviceQueue(renderer->device, renderer->queueFamilies.presentQueueFamilyIndex, 0, &renderer->presentQueue);
    vkGetDeviceQueue(renderer->device, renderer->queueFamilies.transferQueueFamilyIndex, 0, &renderer->transferQueue);

    VkFormat desiredFormats[] = {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB};

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physicalDevice, renderer->surface, &formatCount, 0);
    VkSurfaceFormatKHR formats[16]; // HACK
    vkGetPhysicalDeviceSurfaceFormatsKHR(renderer->physicalDevice, renderer->surface, &formatCount, formats);
    Assert(formats[0].format != VK_FORMAT_UNDEFINED); // Unlike the past, entry value cannot have VK_FORMAT_UNDEFINED format


    VkSurfaceFormatKHR selectedFormat;
    b32 formatAvailable = false;
    for(u32 formatIndex = 0;
        formatIndex < formatCount;
        ++formatIndex)
    {
        for(u32 desiredFormatIndex = 0;
            desiredFormatIndex < ArrayCount(desiredFormats);
            ++desiredFormatIndex)
        {
            if(formats[formatIndex].format == desiredFormats[desiredFormatIndex] && 
                formats[formatIndex].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                selectedFormat = formats[formatIndex];
                formatAvailable = true;
                break;
            }
        }
    }

    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physicalDevice, renderer->surface, &presentModeCount, 0);
    VkPresentModeKHR presentModes[16];
    vkGetPhysicalDeviceSurfacePresentModesKHR(renderer->physicalDevice, renderer->surface, &presentModeCount, presentModes);

    b32 presentModeAvailable = false;
    for(u32 presentModeIndex = 0;
        presentModeIndex < presentModeCount;
        ++presentModeIndex)
    {
        if(presentModes[presentModeIndex] == VK_PRESENT_MODE_FIFO_KHR)
        {
            presentModeAvailable = true;
            break;
        }
    }

    // TODO(joon) : This should not be an assert, maybe find the most similiar format or present mode?
    Assert(formatAvailable && presentModeAvailable);

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    CheckVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer->physicalDevice, renderer->surface, &surfaceCapabilities));

    renderer->surfaceExtent.x = (r32)surfaceCapabilities.currentExtent.width;
    renderer->surfaceExtent.y = (r32)surfaceCapabilities.currentExtent.height;

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    //swapchainCreateInfo.flags; // TODO(joon) : might be worth looking into protected image?
    swapchainCreateInfo.surface = renderer->surface;
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    if(swapchainCreateInfo.minImageCount > surfaceCapabilities.maxImageCount)
    {
        swapchainCreateInfo.minImageCount = surfaceCapabilities.maxImageCount;
    }
    if(surfaceCapabilities.maxImageCount == 0)
    {
        // NOTE(joon) ; If the maxImageCount is 0, means there is no limit for the maximum image count
        swapchainCreateInfo.minImageCount = 3;
    }
    swapchainCreateInfo.imageFormat = selectedFormat.format;
    swapchainCreateInfo.imageColorSpace = selectedFormat.colorSpace;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.imageArrayLayers = 1; // non-stereoscopic
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 1;
    swapchainCreateInfo.pQueueFamilyIndices = &renderer->queueFamilies.graphicsQueueFamilyIndex;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    // TODO(joon) : surfaceCapabilities also return supported alpha composite, so we should check that one too
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; 
    swapchainCreateInfo.clipped = VK_TRUE;

    vkCreateSwapchainKHR(renderer->device, &swapchainCreateInfo, 0, &renderer->swapchain);

    // NOTE(joon) : You _have to_ query first and then get the images, otherwise vulkan always returns VK_INCOMPLETE
    CheckVkResult(vkGetSwapchainImagesKHR(renderer->device, renderer->swapchain, &renderer->swapchainImageCount, 0));
    renderer->swapchainImages = PushArray(platformMemory, VkImage, renderer->swapchainImageCount);
    CheckVkResult(vkGetSwapchainImagesKHR(renderer->device, renderer->swapchain, &renderer->swapchainImageCount, renderer->swapchainImages));

    renderer->swapchainImageViews = PushArray(platformMemory, VkImageView, renderer->swapchainImageCount);
    for(u32 swapchainImageIndex = 0;
        swapchainImageIndex < renderer->swapchainImageCount;
        ++swapchainImageIndex)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = renderer->swapchainImages[swapchainImageIndex];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = selectedFormat.format;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        CheckVkResult(vkCreateImageView(renderer->device, &imageViewCreateInfo, 0, renderer->swapchainImageViews + swapchainImageIndex));
    }

    VkAttachmentDescription renderpassAttachments[2] = {};
    renderpassAttachments[0].format = selectedFormat.format;
    renderpassAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    renderpassAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // This will enable the renderpass clear value
    renderpassAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // If we are not recording the command buffer every single time, OP_STORE should be specified
    renderpassAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderpassAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    renderpassAttachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    renderpassAttachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthStencilAttachment = {};
    renderpassAttachments[1].format = VK_FORMAT_D32_SFLOAT;
    renderpassAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    renderpassAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // This will enable the renderpass clear value
    renderpassAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // If we are not recording the command buffer every single time, OP_STORE should be specified
    renderpassAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderpassAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    renderpassAttachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    renderpassAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // TODO(joon) : Check this value!

    VkAttachmentReference depthStencilReference = {};
    depthStencilReference.attachment = 1;
    depthStencilReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;// Why do we need to specify this twice...
    subpassDescription.pDepthStencilAttachment = &depthStencilReference;

    // TODO(joon) : Add dependency when we need to
#if 0
    VkSubpassDependency dependency = {};
    // NOTE(joon) : if the srcSubpass == dstSubpass, this dependency only defines memory barriers inside the subpass
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
    dependency.srcStageMask = vk_subpass_;
    dependency.dstStageMask;
    dependency.srcAccessMask;
    dependency.dstAccessMask;
    dependency.dependencyFlags;
#endif

    // NOTE(joon): Not an official information, but pipeline is always optimized
    // for using specific subpass with specific renderpass, not for using 
    // multiple subpasses
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = ArrayCount(renderpassAttachments);
    renderPassCreateInfo.pAttachments = renderpassAttachments;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    //renderPassCreateInfo.dependencyCount = 0;
    //renderPassCreateInfo.pDependencies = ;
    
    CheckVkResult(vkCreateRenderPass(renderer->device, &renderPassCreateInfo, 0, &renderer->renderPass));

    platform_read_file_result vertexShaderCode = platformApi->ReadFile("../meka_renderer/code/shader/shader.vert.spv");
    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = {};
    vertexShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleCreateInfo.codeSize = vertexShaderCode.size;
    vertexShaderModuleCreateInfo.pCode = (u32 *)vertexShaderCode.memory;

    VkShaderModule vertexShaderModule;
    CheckVkResult(vkCreateShaderModule(renderer->device, &vertexShaderModuleCreateInfo, 0, &vertexShaderModule));
    //platformApi->FreeFileMemory(vertexShaderCode.memory);

    platform_read_file_result fragmentShaderCode = platformApi->ReadFile("../meka_renderer/code/shader/shader.frag.spv");
    VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo = {};
    fragmentShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleCreateInfo.codeSize = fragmentShaderCode.size;
    fragmentShaderModuleCreateInfo.pCode = (u32 *)fragmentShaderCode.memory;

    VkShaderModule fragmentShaderModule;
    CheckVkResult(vkCreateShaderModule(renderer->device, &fragmentShaderModuleCreateInfo, 0, &fragmentShaderModule));
    //platformApi->FreeFileMemory(fragmentShaderCode.memory);

    VkPipelineShaderStageCreateInfo stageCreateInfo[2] = {};
    stageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stageCreateInfo[0].module = vertexShaderModule;
    stageCreateInfo[0].pName = "main";

    stageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stageCreateInfo[1].module = fragmentShaderModule;
    stageCreateInfo[1].pName = "main";

#if 0
    VkVertexInputBindingDescription vertexBindingDesc = {};
    vertexBindingDesc.binding = 0;
    vertexBindingDesc.stride = sizeof(vertex);
    vertexBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription vertexInputAttribDesc[3] = {};
    vertexInputAttribDesc[0].location = 0;
    vertexInputAttribDesc[0].binding = 0;
    vertexInputAttribDesc[0].format = VK_FORMAT_R32_SFLOAT;
    vertexInputAttribDesc[0].offset = offsetof(vertex, x);

    vertexInputAttribDesc[1].location = 1;
    vertexInputAttribDesc[1].binding = 0;
    vertexInputAttribDesc[1].format = VK_FORMAT_R32_SFLOAT;
    vertexInputAttribDesc[1].offset = offsetof(vertex, y);

    vertexInputAttribDesc[2].location = 2;
    vertexInputAttribDesc[2].binding = 0;
    vertexInputAttribDesc[2].format = VK_FORMAT_R32_SFLOAT;
    vertexInputAttribDesc[2].offset = offsetof(vertex, z);

    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.pVertexBindingDescriptions = &vertexBindingDesc;
    vertexInputState.vertexAttributeDescriptionCount = ArrayCount(vertexInputAttribDesc);
    vertexInputState.pVertexAttributeDescriptions = vertexInputAttribDesc;
#endif
    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {};
    viewport.width = (r32)surfaceCapabilities.currentExtent.width;
    viewport.height = (r32)surfaceCapabilities.currentExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.extent.width = surfaceCapabilities.currentExtent.width;
    scissor.extent.height = surfaceCapabilities.currentExtent.height;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = false;
    rasterizationState.rasterizerDiscardEnable = false; // TODO(joon) :If enabled, the rasterizer will discard some vertices, but don't know what's the policy for that
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_NONE;//VK_CULL_MODE_BACK_BIT; // which triangle should be discarded
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = false;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT ;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthWriteEnable = true;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.depthBoundsTestEnable = false;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.blendEnable = false;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask =   VK_COLOR_COMPONENT_R_BIT |
                                            VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT |
                                            VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.logicOpEnable = false;
    colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;
    
#if 1
    VkDescriptorSetLayoutBinding layoutBinding[2] = {};
    layoutBinding[0].binding = 0;
    layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBinding[0].descriptorCount = 1;
    layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    layoutBinding[1].binding = 1;
    layoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBinding[1].descriptorCount = 1;
    layoutBinding[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;


    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR; // descriptor sets must not be allocated with this layout
    layoutCreateInfo.bindingCount = ArrayCount(layoutBinding);
    layoutCreateInfo.pBindings = layoutBinding;

    VkDescriptorSetLayout setLayout;
    vkCreateDescriptorSetLayout(renderer->device, &layoutCreateInfo, 0, &setLayout);
#endif
    VkPushConstantRange pushConstantRange[1] = {};
    pushConstantRange[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange[0].offset = 0;
    pushConstantRange[0].size = sizeof(m4);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &setLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = ArrayCount(pushConstantRange);  // TODO(joon) : Look into this more to use push constant
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRange;

    CheckVkResult(vkCreatePipelineLayout(renderer->device, &pipelineLayoutCreateInfo, 0, &renderer->pipelineLayout));

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }; 
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO; 
    dynamicState.dynamicStateCount = ArrayCount(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;


    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = stageCreateInfo;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputState; // vertices are pushed using pushDescriptorSet
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    graphicsPipelineCreateInfo.pViewportState = &viewportState;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationState;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleState;
    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilState;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendState;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicState;
    graphicsPipelineCreateInfo.layout = renderer->pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = renderer->renderPass;
    graphicsPipelineCreateInfo.subpass = 0;

    CheckVkResult(vkCreateGraphicsPipelines(renderer->device, 0, 1, &graphicsPipelineCreateInfo, 0, &renderer->pipeline));

    vkDestroyShaderModule(renderer->device, vertexShaderModule, 0);
    vkDestroyShaderModule(renderer->device, fragmentShaderModule, 0);

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_D32_SFLOAT;
    imageCreateInfo.extent.width = surfaceCapabilities.currentExtent.width;
    imageCreateInfo.extent.height = surfaceCapabilities.currentExtent.height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    CheckVkResult(vkCreateImage(renderer->device, &imageCreateInfo, 0, &renderer->depthImage));

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(renderer->device, renderer->depthImage, &memoryRequirements);

    // TODO(joon) : Store this value!
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(renderer->physicalDevice, &physicalDeviceMemoryProperties);

    u32 memoryTypeBits = memoryRequirements.memoryTypeBits;
    u32 memoryTypeIndex = 0;
    b32 isMemoryTypeSupported = false;
    while(memoryTypeBits)
    {
        if(memoryTypeBits & 1)
        {
            if((physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                isMemoryTypeSupported = true;
                break;
            }
        }

        memoryTypeBits = memoryTypeBits >> 1;
        memoryTypeIndex++;
    }
    Assert(isMemoryTypeSupported);

    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = memoryTypeIndex;
    CheckVkResult(vkAllocateMemory(renderer->device, &memoryAllocInfo, 0, &renderer->depthImageMemory));

    CheckVkResult(vkBindImageMemory(renderer->device, renderer->depthImage, renderer->depthImageMemory, 0));

    VkImageViewCreateInfo imageViewCreateInfo = {};
    imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreateInfo.image = renderer->depthImage;
    imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreateInfo.format = VK_FORMAT_D32_SFLOAT;
    imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    CheckVkResult(vkCreateImageView(renderer->device, &imageViewCreateInfo, 0, &renderer->depthImageView));

    renderer->framebuffers = PushArray(platformMemory, VkFramebuffer, renderer->swapchainImageCount);
    for(u32 framebufferIndex = 0;
        framebufferIndex < renderer->swapchainImageCount;
        ++framebufferIndex)
    {
        VkImageView attachments[2] = {};
        attachments[0] = renderer->swapchainImageViews[framebufferIndex];
        attachments[1] = renderer->depthImageView;

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderer->renderPass;
        framebufferCreateInfo.attachmentCount = ArrayCount(attachments);
        framebufferCreateInfo.pAttachments = attachments;
        framebufferCreateInfo.width = surfaceCapabilities.currentExtent.width;
        framebufferCreateInfo.height = surfaceCapabilities.currentExtent.height;
        framebufferCreateInfo.layers = 1;

        CheckVkResult(vkCreateFramebuffer(renderer->device, &framebufferCreateInfo, 0, renderer->framebuffers + framebufferIndex));
    }
    
    // NOTE(joon) : command buffers inside the pool can only be submitted to the specified queue
    // graphics related command buffers -> graphics queue
    // transfer related command buffers -> transfer queue
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = renderer->queueFamilies.graphicsQueueFamilyIndex;

    CheckVkResult(vkCreateCommandPool(renderer->device, &commandPoolCreateInfo, 0, &renderer->graphicsCommandPool));

    u32 commandBufferCount = renderer->swapchainImageCount;
    renderer->graphicsCommandBuffers = PushArray(platformMemory, VkCommandBuffer, renderer->swapchainImageCount);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = renderer->graphicsCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = commandBufferCount;

    CheckVkResult(vkAllocateCommandBuffers(renderer->device, &commandBufferAllocateInfo, renderer->graphicsCommandBuffers));

    // TODO(joon) : If possible, make this to use transfer queue(and command pool)!
    VkCommandBuffer transferCommandBuffer;
    commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = renderer->graphicsCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    CheckVkResult(vkAllocateCommandBuffers(renderer->device, &commandBufferAllocateInfo, &transferCommandBuffer));
    
    renderer->imageReadyToRenderSemaphores = PushArray(platformMemory, VkSemaphore, 2*renderer->swapchainImageCount);
    renderer->imageReadyToPresentSemaphores = renderer->imageReadyToRenderSemaphores + renderer->swapchainImageCount;
    for(u32 semaphoreIndex = 0;
        semaphoreIndex < renderer->swapchainImageCount;
        ++semaphoreIndex)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        CheckVkResult(vkCreateSemaphore(renderer->device, &semaphoreCreateInfo, 0, renderer->imageReadyToRenderSemaphores + semaphoreIndex));
        CheckVkResult(vkCreateSemaphore(renderer->device, &semaphoreCreateInfo, 0, renderer->imageReadyToPresentSemaphores + semaphoreIndex));
    }

    renderer->imageReadyToRenderFences = PushArray(platformMemory, VkFence, 2*renderer->swapchainImageCount);
    renderer->imageReadyToPresentFences = renderer->imageReadyToRenderFences + renderer->swapchainImageCount;
    for(u32 fenceIndex = 0;
        fenceIndex < renderer->swapchainImageCount;
        ++fenceIndex)
    {
        VkFenceCreateInfo  fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        
        CheckVkResult(vkCreateFence(renderer->device, &fenceCreateInfo, 0, renderer->imageReadyToRenderFences + fenceIndex));
        CheckVkResult(vkCreateFence(renderer->device, &fenceCreateInfo, 0, renderer->imageReadyToPresentFences + fenceIndex));
    }

    // TODO(joon) : Correct this image memory barrier, and also find out what pipeline stage we should be using
#if 0
    for(u32 swapchainImageIndex = 0;
        swapchainImageIndex < renderer->swapchainImageCount;
        ++swapchainImageIndex)
    {
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE_KHR;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // No queue transfer occurs
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.image = swapchainImages[swapchainImageIndex];
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = 1; // vk_remaining_mip_levels?
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 1; // vk_remaining_array_levels?
        
        vkCmdPipelineBarrier(transferCommandBuffer,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                    0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    1,
                                    &imageMemoryBarrier);

    }
#endif
}

internal void
CleanVulkan(renderer *renderer)
{
    vkDeviceWaitIdle(renderer->device);
    for(u32 swapchainImageIndex = 0;
        swapchainImageIndex < renderer->swapchainImageCount;
        ++swapchainImageIndex)
    {
        vkDestroySemaphore(renderer->device, renderer->imageReadyToPresentSemaphores[swapchainImageIndex], 0);
        vkDestroySemaphore(renderer->device, renderer->imageReadyToPresentSemaphores[swapchainImageIndex], 0);
        vkDestroyFence(renderer->device, renderer->imageReadyToPresentFences[swapchainImageIndex], 0);
        vkDestroyFence(renderer->device, renderer->imageReadyToPresentFences[swapchainImageIndex], 0);
    }

    vkFreeCommandBuffers(renderer->device, renderer->graphicsCommandPool, renderer->swapchainImageCount, renderer->graphicsCommandBuffers);

    vkDestroyCommandPool(renderer->device, renderer->graphicsCommandPool, 0);

    for(u32 swapchainImageIndex = 0;
        swapchainImageIndex < renderer->swapchainImageCount;
        ++swapchainImageIndex)
    {
        vkDestroyFramebuffer(renderer->device, renderer->framebuffers[swapchainImageIndex], 0);
    }

    vkFreeMemory(renderer->device, renderer->depthImageMemory, 0);
    vkDestroyImageView(renderer->device, renderer->depthImageView, 0);
    vkDestroyImage(renderer->device, renderer->depthImage, 0);

    vkDestroyPipelineLayout(renderer->device, renderer->pipelineLayout, 0);
    vkDestroyPipeline(renderer->device, renderer->pipeline, 0);
    vkDestroyRenderPass(renderer->device, renderer->renderPass, 0);

    for(u32 swapchainImageIndex = 0;
        swapchainImageIndex < renderer->swapchainImageCount;
        ++swapchainImageIndex)
    {
        vkDestroyImageView(renderer->device, renderer->swapchainImageViews[swapchainImageIndex], 0);
    }

    // NOTE(joon) : swap chain images themselves wil be destorye with swapchain
    vkDestroySwapchainKHR(renderer->device, renderer->swapchain, 0);

    vkDestroySurfaceKHR(renderer->instance, renderer->surface, 0);

    vkDestroyDevice(renderer->device, 0);
    vkDestroyInstance(renderer->instance, 0);

}

    



