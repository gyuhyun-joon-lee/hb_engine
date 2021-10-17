/*
 * Written by Gyuhyun 'Joon' Lee
 */

#define CheckVkResult(expression)                                             \
{                                                                             \
    VkResult vkResult = (expression);                                           \
    if(vkResult != VK_SUCCESS)                                                  \
    {                                                                           \
        Assert(0);                                                              \
    }                                                                           \
}

struct vertex
{
    r32 x;
    r32 y;
    r32 z;
};

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



internal VKAPI_ATTR VkBool32 VKAPI_CALL 
DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, 
                        VkDebugUtilsMessageTypeFlagsEXT types,
                        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                        void* userData)
{
    write_console_info *writeConsoleInfo = (write_console_info *)userData;
    if(writeConsoleInfo->console && writeConsoleInfo->WriteConsole)
    {
        u32 charCount = GetCharCountNullTerminated(callbackData->pMessage);
        Assert(charCount >= 0);
        writeConsoleInfo->WriteConsole(writeConsoleInfo->console, callbackData->pMessage, charCount);
    }

    return true;
}

internal void
CreateDebugMessenger(VkInstance instance, write_console_info *writeConsoleInfo)
{
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
    
    debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessengerInfo.pNext = 0;
    debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ;
    debugMessengerInfo.messageType = //VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugMessengerInfo.pfnUserCallback = DebugMessengerCallback;
    debugMessengerInfo.pUserData = (void *)writeConsoleInfo;

    VkDebugUtilsMessengerEXT messenger;
    CheckVkResult(vkCreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, 0, &messenger));
}

struct vk_queue_families
{
    u32 graphicsQueueFamilyIndex;
    u32 graphicsQueueCount;
    u32 presentQueueFamilyIndex;
    u32 presentQueueCount;
    u32 transferQueueFamilyIndex;
    u32 transferQueueCount;
};

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
        "VK_KHR_swapchain"
    };

    // NOTE(joon) : device layers are deprecated
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    //deviceCreateInfo.queueCreateInfoCount = 1;
    //deviceCreateInfo.pQueueCreateInfos = &queueCreateInfos;
    deviceCreateInfo.queueCreateInfoCount = validQueueCreateInfoCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.enabledExtensionCount = ArrayCount(deviceExtensionNames);
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;
    deviceCreateInfo.pEnabledFeatures = 0; // TODO(joon) : enable best practice feature!

    VkDevice device;
    CheckVkResult(vkCreateDevice(physicalDevice, &deviceCreateInfo, 0, &device));

    return device;
}

internal void
Win32InitVulkan(HINSTANCE instanceHandle, HWND windowHandle, i32 screenWidth, i32 screenHeight,
                write_console_info *writeConsoleInfo, platform_api *platformApi)
{
    ResolvePreInstanceLevelFunctions();

    u32 layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, 0);
    VkLayerProperties layers[64];
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
            if(StringCompare(instanceLayerNames[desiredLayerIndex], layers[layerIndex].layerName))
            {
                found = true;
                break;
            }
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
        "VK_KHR_win32_surface"
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

    VkInstance instance;
    CheckVkResult(vkCreateInstance(&instanceCreateInfo, 0, &instance));

    ResolveInstanceLevelFunctions(instance);

#if MEKA_DEBUG
    CreateDebugMessenger(instance, writeConsoleInfo);
#endif

    VkPhysicalDevice physicalDevice = FindPhysicalDevice(instance);

    VkSurfaceKHR surface;
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hinstance = instanceHandle;
    surfaceCreateInfo.hwnd = windowHandle;
    CheckVkResult(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, 0, &surface));

    vk_queue_families queueFamilies = GetSupportedQueueFamilies(physicalDevice, surface);
    VkDevice device = CreateDevice(physicalDevice, surface, &queueFamilies);
    ResolveDeviceLevelFunctions(device);

    VkQueue graphicsQueue;
    VkQueue transferQueue;
    VkQueue presentQueue;
    vkGetDeviceQueue(device, queueFamilies.graphicsQueueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, queueFamilies.presentQueueFamilyIndex, 0, &presentQueue);
    vkGetDeviceQueue(device, queueFamilies.transferQueueFamilyIndex, 0, &transferQueue);

    VkFormat desiredFormats[] = {VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB};

    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, 0);
    VkSurfaceFormatKHR formats[16]; // HACK
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats);

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
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, 0);
    VkPresentModeKHR presentModes[16];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes);

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
    CheckVkResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    //swapchainCreateInfo.flags; // TODO(joon) : might be worth looking into protected image?
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1; // TODO(joon) : Also consider maxImageCount to be more robust
    swapchainCreateInfo.imageFormat = selectedFormat.format;
    swapchainCreateInfo.imageColorSpace = selectedFormat.colorSpace;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    swapchainCreateInfo.imageArrayLayers = 1; // non-stereoscopic
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainCreateInfo.queueFamilyIndexCount = 1;
    swapchainCreateInfo.pQueueFamilyIndices = &queueFamilies.graphicsQueueFamilyIndex;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;

    VkSwapchainKHR swapchain;
    vkCreateSwapchainKHR(device, &swapchainCreateInfo, 0, &swapchain);

    u32 swapchainImageCount = 0;
    VkImage swapchainImages[256]; // HACK : Properly query the images 
    // NOTE(joon) : You _have to_ query first and then get the images, otherwise vulkan always returns VK_INCOMPLETE
    CheckVkResult(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, 0));
    CheckVkResult(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages));

    Assert(swapchainImageCount <= ArrayCount(swapchainImages));

    VkImageView swapchainImageViews[16];
    for(u32 swapchainImageIndex = 0;
        swapchainImageIndex < swapchainImageCount;
        ++swapchainImageIndex)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {};
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.image = swapchainImages[swapchainImageIndex];
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = selectedFormat.format;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        CheckVkResult(vkCreateImageView(device, &imageViewCreateInfo, 0, swapchainImageViews + swapchainImageIndex));
    }

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = selectedFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentReference = {};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // TODO(joon) : Check this value!

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorAttachmentReference;// Why do we need to specify this twice...

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
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    //renderPassCreateInfo.dependencyCount = 0;
    //renderPassCreateInfo.pDependencies = ;
    
    VkRenderPass renderPass;
    CheckVkResult(vkCreateRenderPass(device, &renderPassCreateInfo, 0, &renderPass));

    platform_read_file_result vertexShaderCode = platformApi->ReadFile("../meka_renderer/code/shader/shader.vert.spv");
    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = {};
    vertexShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vertexShaderModuleCreateInfo.codeSize = vertexShaderCode.size;
    vertexShaderModuleCreateInfo.pCode = (u32 *)vertexShaderCode.memory;

    VkShaderModule vertexShaderModule;
    CheckVkResult(vkCreateShaderModule(device, &vertexShaderModuleCreateInfo, 0, &vertexShaderModule));
    //platformApi->FreeFileMemory(vertexShaderCode.memory);

    platform_read_file_result fragmentShaderCode = platformApi->ReadFile("../meka_renderer/code/shader/shader.frag.spv");
    VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo = {};
    fragmentShaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fragmentShaderModuleCreateInfo.codeSize = fragmentShaderCode.size;
    fragmentShaderModuleCreateInfo.pCode = (u32 *)fragmentShaderCode.memory;

    VkShaderModule fragmentShaderModule;
    CheckVkResult(vkCreateShaderModule(device, &fragmentShaderModuleCreateInfo, 0, &fragmentShaderModule));
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

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport = {};
    viewport.width = (r32)surfaceCapabilities.currentExtent.width;
    viewport.height = (r32)surfaceCapabilities.currentExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissorRect = {};
    scissorRect.extent.width = surfaceCapabilities.currentExtent.width;
    scissorRect.extent.height = surfaceCapabilities.currentExtent.height;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissorRect;

    VkPipelineRasterizationStateCreateInfo rasterizationState = {};
    rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationState.depthClampEnable = false;
    rasterizationState.rasterizerDiscardEnable = false; // to use msaa, this should be false
    rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT; // which triangle should be discarded
    rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationState.depthBiasEnable = false;
    rasterizationState.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisampleState = {};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT ;

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = true;
    depthStencilState.depthWriteEnable = false;
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
#if 0
    VkDescriptorSetLayoutBinding layoutBinding = {};
    uint32_t              binding;
    VkDescriptorType      descriptorType;
    uint32_t              descriptorCount;
    VkShaderStageFlags    stageFlags;
    const VkSampler*      pImmutableSamplers;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.flags = 0; // TODO(joon) : might wanna look into this more, for push descriptor set 
    layoutCreateInfo.bindingCount = 0; // TODO(joon) : Add setLayoutBinding here!
    layoutCreateInfo.pBindings = 0;

    // TODO(joon) : add descriptionSetLayout here!
    vkCreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
#endif

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = 0;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;  // TODO(joon) : Look into this more to use push constant
    pipelineLayoutCreateInfo.pPushConstantRanges = 0;

    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }; 
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO; 
    dynamicState.dynamicStateCount = ArrayCount(dynamicStates);
    dynamicState.pDynamicStates = dynamicStates;

    VkPipelineLayout pipelineLayout;
    CheckVkResult(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, 0, &pipelineLayout));

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
    graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = stageCreateInfo;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputState;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    graphicsPipelineCreateInfo.pViewportState = &viewportState;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationState;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleState;
    graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilState;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendState;
    graphicsPipelineCreateInfo.pDynamicState = &dynamicState;
    graphicsPipelineCreateInfo.layout = pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = renderPass;
    graphicsPipelineCreateInfo.subpass = 0;

    VkPipeline pipeline;
    CheckVkResult(vkCreateGraphicsPipelines(device, 0, 1, &graphicsPipelineCreateInfo, 0, &pipeline));

    vkDestroyShaderModule(device, vertexShaderModule, 0);
    vkDestroyShaderModule(device, fragmentShaderModule, 0);

    VkFramebuffer framebuffers[16]; // HACK
    for(u32 framebufferIndex = 0;
        framebufferIndex < swapchainImageCount;
        ++framebufferIndex)
    {
        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = 1;
        framebufferCreateInfo.pAttachments = swapchainImageViews + framebufferIndex;
        framebufferCreateInfo.width = surfaceCapabilities.currentExtent.width;
        framebufferCreateInfo.height = surfaceCapabilities.currentExtent.height;
        framebufferCreateInfo.layers = 1;

        CheckVkResult(vkCreateFramebuffer(device, &framebufferCreateInfo, 0, framebuffers + framebufferIndex));
    }
    
    // NOTE(joon) : command buffers inside the pool can only be submitted to the specified queue
    // graphics related command buffers -> graphics queue
    // transfer related command buffers -> transfer queue
    VkCommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    //commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCreateInfo.queueFamilyIndex = queueFamilies.graphicsQueueFamilyIndex;

    VkCommandPool graphicsCommandPool;
    CheckVkResult(vkCreateCommandPool(device, &commandPoolCreateInfo, 0, &graphicsCommandPool));

    u32 commandBufferCount = swapchainImageCount;
    VkCommandBuffer swapchainGraphicsCommandBuffers[16];

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = graphicsCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = commandBufferCount;

    CheckVkResult(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, swapchainGraphicsCommandBuffers));

    // TODO(joon) : If possible, make this to use transfer queue(and command pool)!
    VkCommandBuffer transferCommandBuffer;
    commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = graphicsCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferAllocateInfo.commandBufferCount = 1;
    CheckVkResult(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &transferCommandBuffer));

    
    VkSemaphore imageReadyToRenderSemaphores[16] = {};// HACK
    VkSemaphore imageReadyToPresentSemaphores[16] = {};// HACK
    for(u32 semaphoreIndex = 0;
        semaphoreIndex < swapchainImageCount;
        ++semaphoreIndex)
    {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {};
        semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        CheckVkResult(vkCreateSemaphore(device, &semaphoreCreateInfo, 0, imageReadyToRenderSemaphores + semaphoreIndex));
        CheckVkResult(vkCreateSemaphore(device, &semaphoreCreateInfo, 0, imageReadyToPresentSemaphores + semaphoreIndex));
    }

    VkFence imageReadyToRenderFences[16] = {}; // HACK
    VkFence imageReadyToPresentFences[16] = {}; // HACK
    for(u32 fenceIndex = 0;
        fenceIndex < swapchainImageCount;
        ++fenceIndex)
    {
        VkFenceCreateInfo  fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        
        CheckVkResult(vkCreateFence(device, &fenceCreateInfo, 0, imageReadyToRenderFences + fenceIndex));
        CheckVkResult(vkCreateFence(device, &fenceCreateInfo, 0, imageReadyToPresentFences + fenceIndex));
    }

    vertex vertices[3] = {};
    vertices[0] = {-0.5f, -0.5f, 0.0f};
    vertices[1] = {0.0f, 0.5f, 0.0f};
    vertices[2] = {0.5f, -0.5f, 0.0f};

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = sizeof(vertex) * ArrayCount(vertices);
    bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer vertexBuffer;
    CheckVkResult(vkCreateBuffer(device, &bufferCreateInfo, 0, &vertexBuffer));

    VkMemoryRequirements memoryRequirements = {};
    vkGetBufferMemoryRequirements(device, vertexBuffer, &memoryRequirements);

    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

    // TODO(joon) : check whether the physical device actually supports the memory type or not!

    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = 0;

    VkDeviceMemory vertexBufferMemory;
    CheckVkResult(vkAllocateMemory(device, &memoryAllocInfo, 0, &vertexBufferMemory));
    CheckVkResult(vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0));

    VkCommandBufferBeginInfo commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CheckVkResult(vkBeginCommandBuffer(transferCommandBuffer, &commandBufferBeginInfo));

    vkCmdUpdateBuffer(transferCommandBuffer, vertexBuffer, 0, sizeof(vertex)*ArrayCount(vertices), vertices);

    // TODO(joon) : Correct this image memory barrier, and also find out what pipeline stage we should be using
#if 0
    for(u32 swapchainImageIndex = 0;
        swapchainImageIndex < swapchainImageCount;
        ++swapchainImageIndex)
    {
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE_KHR;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        imageMemoryBarrier.srcQueueFamilyIndex = queueFamilies.graphicsQueueFamilyIndex;
        imageMemoryBarrier.dstQueueFamilyIndex = imageMemoryBarrier.srcQueueFamilyIndex;
        imageMemoryBarrier.image = swapchainImages[swapchainImageIndex];
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.levelCount = 1;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 1;
        
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
    vkEndCommandBuffer(transferCommandBuffer);
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &transferCommandBuffer;

    CheckVkResult(vkQueueSubmit(graphicsQueue, 1, &submitInfo, 0));
    CheckVkResult(vkQueueWaitIdle(graphicsQueue));

    // TODO(joon)VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT

    for(u32 swapchainImageIndex = 0;
        swapchainImageIndex < swapchainImageCount;
        ++swapchainImageIndex)
    {
        VkCommandBuffer *commandBuffer = swapchainGraphicsCommandBuffers + swapchainImageIndex;
        
        // TODO(joon) : fence needed here!
        commandBufferBeginInfo = {};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        CheckVkResult(vkBeginCommandBuffer(*commandBuffer, &commandBufferBeginInfo));

        vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(*commandBuffer, 0, 1, &scissorRect);

        VkClearValue clearValues[2] = {}; 
        clearValues[0].color = {0.1f, 0.1f, 0.1f, 1};
        clearValues[1].depthStencil.depth = 1.0f;
        //clearValues[1].depthStencil;
        VkRenderPassBeginInfo renderPassBeginInfo = {};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = renderPass;
        renderPassBeginInfo.framebuffer = framebuffers[swapchainImageIndex];
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = surfaceCapabilities.currentExtent;
        renderPassBeginInfo.clearValueCount = ArrayCount(clearValues);
        renderPassBeginInfo.pClearValues = clearValues;
        vkCmdBeginRenderPass(*commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS , pipeline);

        VkDeviceSize offset = 0; 
        vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &vertexBuffer, &offset);

        vkCmdDraw(*commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(*commandBuffer);
        CheckVkResult(vkEndCommandBuffer(*commandBuffer));
    }

    u32 currentFrameIndex = 0;
    while(1)
    {
        // TODO(joon) : Check whether this fence is working correctly! only one fence might be enough?
        if(vkGetFenceStatus(device, imageReadyToRenderFences[currentFrameIndex]) == VK_SUCCESS)
        {
            // The image that we are trying to get might not be ready inside GPU, 
            // and CPU can only know about it using fence.
            vkWaitForFences(device, 1, &imageReadyToRenderFences[currentFrameIndex], true, UINT64_MAX);
            vkResetFences(device, 1, &imageReadyToRenderFences[currentFrameIndex]); // Set fences to unsignaled state
        }

        u32 imageIndex = UINT_MAX;
        VkResult acquireNextImageResult = vkAcquireNextImageKHR(device, swapchain, 
                                            UINT64_MAX, // TODO(joon) : Why not just use the next image, instead of waiting this image?
                                            imageReadyToRenderSemaphores[currentFrameIndex], 
                                            imageReadyToRenderFences[currentFrameIndex], // signaled when the application can use the image.
                                            &imageIndex);

        if(acquireNextImageResult == VK_SUCCESS)
        {
            VkCommandBuffer *commandBuffer = swapchainGraphicsCommandBuffers + imageIndex;

            VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            VkSubmitInfo queueSubmitInfo = {};
            queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            queueSubmitInfo.waitSemaphoreCount = 1;
            queueSubmitInfo.pWaitSemaphores = imageReadyToRenderSemaphores + currentFrameIndex;
            queueSubmitInfo.pWaitDstStageMask = &waitFlags;
            queueSubmitInfo.commandBufferCount = 1;
            queueSubmitInfo.pCommandBuffers = commandBuffer;
            queueSubmitInfo.signalSemaphoreCount = 1;
            queueSubmitInfo.pSignalSemaphores = imageReadyToPresentSemaphores + currentFrameIndex;

            CheckVkResult(vkQueueSubmit(graphicsQueue, 1, &queueSubmitInfo, imageReadyToPresentFences[currentFrameIndex]));

            // NOTE(joon) : After we submit to the queue, wait until the fence is ready
            if(vkGetFenceStatus(device, imageReadyToPresentFences[currentFrameIndex]) != VK_SUCCESS) // waiting for the fence to be signaled
            {
                vkWaitForFences(device, 1, &imageReadyToPresentFences[currentFrameIndex], true, UINT64_MAX);
                vkResetFences(device, 1, &imageReadyToPresentFences[currentFrameIndex]); // Set fences to unsignaled state
            }

            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = imageReadyToPresentSemaphores + currentFrameIndex;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapchain;
            presentInfo.pImageIndices = &imageIndex;

            CheckVkResult(vkQueuePresentKHR(graphicsQueue, &presentInfo));

            // NOTE(joon) : equivalent to calling vkQueueWaitIdle for all queues
            CheckVkResult(vkDeviceWaitIdle(device));
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

        currentFrameIndex = (currentFrameIndex + 1)%swapchainImageCount;
    }
}



