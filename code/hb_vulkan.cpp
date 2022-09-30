/*
 * Written by Gyuhyun Lee
 */

#include "hb_vulkan.h"

#include <stdio.h>
#define CheckVkResult(expression)                                             \
{                                                                             \
    VkResult vkResult = (expression);                                           \
    if(vkResult != VK_SUCCESS)                                                  \
    {                                                                           \
        printf("CheckVkResult Failed with vkResult : %i\n", vkResult);                               \
        assert(0);                                                              \
    }                                                                           \
}                                                                               \

internal VkPhysicalDevice
find_physical_device(VkInstance instance)
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

internal void
vk_check_desired_layers_support(char **desired_layers, u32 desired_layer_count)
{
    u32 layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, 0);
    VkLayerProperties layers[64]; // TODO(joon) use variable legnth array for this
    vkEnumerateInstanceLayerProperties(&layer_count, layers);

    for(u32 desired_layer_index = 0;
            desired_layer_index < array_count(desired_layers);
            ++desired_layer_index)
    {
        char **desired_layer = desired_layers + desired_layer_index;

        b32 found = false;
        for(u32 layer_index = 0;
                layer_index < layer_count;
                ++layer_index)
        {
            if(strcmp(*desired_layer, layers[layer_index].layerName) == 0)
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            printf("%s not found", *desired_layer);
            assert(0);
        }
    }
}

internal void
vk_check_desired_extensions_support(char **desired_extensions, u32 desired_extension_count)
{
    u32 extension_count;
    vkEnumerateInstanceExtensionProperties(0, &extension_count, 0);
    VkExtensionProperties extensions[64];
    vkEnumerateInstanceExtensionProperties(0, &extension_count, extensions);

    for(u32 desired_extension_index = 0;
        desired_extension_index < desired_extension_count;
        ++desired_extension_index)
    {
        char **desired_extension = desired_extensions + desired_extension_index;
        
        b32 found = false;
        for(u32 extension_index = 0;
                extension_index < extension_count;
                ++extension_index)
        {
            if(strcmp(*desired_extension, extensions[extension_index].extensionName) == 0)
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            printf("%s not found", *desired_extension);
            assert(0);
        }
    }
}

// TODO(joon) Do we want to check more than one present mode?
internal void
vk_check_desired_present_mode_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface, VkPresentModeKHR desired_present_mode)
{
    u32 presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, 0);
    VkPresentModeKHR presentModes[16];
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &presentModeCount, presentModes);

    b32 found = false;
    for(u32 presentModeIndex = 0;
        presentModeIndex < presentModeCount;
        ++presentModeIndex)
    {
        if(presentModes[presentModeIndex] == VK_PRESENT_MODE_FIFO_KHR)
        {
            found = true;
            break;
        }

        if(!found)
        {
            printf("present mode not avilable");
            assert(0);
        }
    }
}

// returns an 'usable' index to the physical_device_memory_properties.memoryTypes
// that supports the desired memory properties
internal u32
vk_get_usable_memory_index(VkPhysicalDevice physical_device, VkDevice device, 
                            VkMemoryRequirements memory_requirements, VkMemoryPropertyFlags desired_memory_properties)
{
    u32 result = u32_max;

    // TODO(joon) As long as we don't change the physical device, we can store this value
    VkPhysicalDeviceMemoryProperties physical_device_memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &physical_device_memory_properties);

    u32 possible_memory_types = memory_requirements.memoryTypeBits;

    // TODO(joon) can we pull this out into a seperate function?
    u32 memory_type_index = 0;

    while(possible_memory_types)
    {
        if(possible_memory_types & (1 << memory_type_index)) // if the least significant bit was set, 
        {
            if((physical_device_memory_properties.memoryTypes[memory_type_index].propertyFlags & desired_memory_properties) == desired_memory_properties)
            {
                result = memory_type_index;
                break;
            }
        }

        memory_type_index++;
    }
    assert(result != u32_max);

    return result;
}

internal VkSurfaceFormatKHR
vk_select_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface, 
                        VkFormat *format_candidates, u32 format_candidate_count)
{
    u32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, 0);
    VkSurfaceFormatKHR formats[16]; // HACK
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats);
    assert(formats[0].format != VK_FORMAT_UNDEFINED); // Unlike the past, entry value cannot have VK_FORMAT_UNDEFINED format

    VkSurfaceFormatKHR result = {};
    for(u32 formatIndex = 0;
            formatIndex < format_count;
            ++formatIndex)
    {
        for(u32 candidate_format_index = 0;
                candidate_format_index < format_candidate_count;
                ++candidate_format_index)
        {
            if(formats[formatIndex].format == format_candidates[candidate_format_index] && 
                formats[formatIndex].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                result = formats[formatIndex];
                break;
            }
        }
    }
    
    if(!result.format)
    {
        printf("Could not find the surface with desired format!\n");
        assert(0);
    }

    return result;
}

internal VkInstance
vk_create_instance(char **layers, u32 layer_count, char **extensions, u32 extension_count)
{
    VkInstanceCreateInfo instance_create_info = {};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = 0;
    instance_create_info.flags = 0;
    instance_create_info.enabledLayerCount = layer_count;
    instance_create_info.ppEnabledLayerNames = layers;
    instance_create_info.enabledExtensionCount = extension_count;
    instance_create_info.ppEnabledExtensionNames = extensions;

    VkInstance instance;
    CheckVkResult(vkCreateInstance(&instance_create_info, 0, &instance));
    
    return instance;
}

internal VKAPI_ATTR VkBool32 VKAPI_CALL 
DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, 
                        VkDebugUtilsMessageTypeFlagsEXT types,
                        const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                        void* userData)
{
    // TODO(joon) put this back
    printf("%s", (char *)callbackData->pMessage);

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
get_supported_queue_familes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
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
vk_create_device(VkPhysicalDevice physical_device, VkSurfaceKHR surface, vk_queue_families *queueFamilies)
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
#if 0
        VK_KHR_8BIT_STORAGE_EXTENSION_NAME, // TODO(joon) : This is not supported on every devices
        VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
#endif
        //VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
        "VK_KHR_portability_subset",
        VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
    };

    u32 propertyCount = 0;
    CheckVkResult(vkEnumerateDeviceExtensionProperties(physical_device, 0, &propertyCount, 0));
    VkExtensionProperties availableDeviceExtensions[256]; // HACK
    CheckVkResult(vkEnumerateDeviceExtensionProperties(physical_device, 0, &propertyCount, availableDeviceExtensions));

#if 0
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
#endif

    VkPhysicalDeviceFeatures2 feature2 = {};
    feature2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    feature2.pNext = 0;

    // NOTE(joon) : device layers are deprecated
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &feature2;
    deviceCreateInfo.queueCreateInfoCount = validQueueCreateInfoCount;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.enabledExtensionCount = array_count(deviceExtensionNames);
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;

    VkDevice device;
    CheckVkResult(vkCreateDevice(physical_device, &deviceCreateInfo, 0, &device));

    return device;
}

#if 1
internal VkDeviceMemory
VkBufferAllocateMemory(VkPhysicalDevice physical_device, VkDevice device, 
                    VkBuffer buffer, VkMemoryPropertyFlags flags)
{
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

    u32 usable_memory_index = vk_get_usable_memory_index(physical_device, device, 
                                memory_requirements, flags);

    // TODO(joon) : check whether the physical device actually supports the memory type or not
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memory_requirements.size;
    memoryAllocInfo.memoryTypeIndex = usable_memory_index;

    VkDeviceMemory bufferMemory;
    CheckVkResult(vkAllocateMemory(device, &memoryAllocInfo, 0, &bufferMemory));
    CheckVkResult(vkBindBufferMemory(device, buffer, bufferMemory, 0));

    return bufferMemory;
}
#endif

internal vk_uniform_buffer
vk_create_uniform_buffer(VkPhysicalDevice physical_device, VkDevice device, 
                        u32 element_size, u32 element_count, u32 alignment)
{
    vk_uniform_buffer result = {};

    result.element_size = ((element_size / alignment) + 1) * alignment;;
    result.element_count = element_count;

    u32 total_size = result.element_size * result.element_count;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = total_size;
    bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // NOTE(joon) exculsive to this family

    CheckVkResult(vkCreateBuffer(device, &bufferCreateInfo, 0, &result.buffer));
    result.device_memory = VkBufferAllocateMemory(physical_device, device, result.buffer, 
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    CheckVkResult(vkMapMemory(device, result.device_memory, 0, total_size, 0, &result.memory));

    return result;
}

internal void
vk_update_uniform_buffer(vk_uniform_buffer *uniform_buffer, u32 index, void *source, u32 source_size)
{
    assert(index < uniform_buffer->element_count);
    memcpy((u8 *)uniform_buffer->memory + uniform_buffer->element_size * index, source, source_size);
}

internal void
vk_update_host_visible_coherent_buffer_entirely(vk_host_visible_coherent_buffer *buffer, void *source, u32 source_size)
{
    memcpy((u8 *)buffer->memory, source, source_size);
}

internal vk_host_visible_coherent_buffer
vk_create_host_visible_coherent_buffer(VkPhysicalDevice physical_device, VkDevice device, VkBufferUsageFlags usage, void *source, u32 source_size)
{
    vk_host_visible_coherent_buffer result = {};
    result.size = source_size;

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = result.size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // NOTE(joon) exculsive to this family

    CheckVkResult(vkCreateBuffer(device, &bufferCreateInfo, 0, &result.buffer));
    result.device_memory = VkBufferAllocateMemory(physical_device, device, result.buffer, 
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    // This pointer minus offset must be aligned to at least VkPhysicalDeviceLimits::minMemoryMapAlignment.
    CheckVkResult(vkMapMemory(device, result.device_memory, 0, result.size, 0, &result.memory));

    vk_update_host_visible_coherent_buffer_entirely(&result, source, source_size);

    return result;
}


#if 0
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
#endif

internal void
vk_set_viewport(VkCommandBuffer command_buffer, u32 width, u32 height, f32 min, f32 max)
{
    VkViewport viewport = {};
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = min;
    viewport.maxDepth = max;

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
}

internal void
vk_set_scissor_rect(VkCommandBuffer command_buffer, u32 width, u32 height)
{
    VkRect2D scissor_rect = {};
    scissor_rect.extent.width = width;
    scissor_rect.extent.height = height;

    vkCmdSetScissor(command_buffer, 0, 1, &scissor_rect);
}

#if 1
internal RenderMesh
vk_create_render_mesh(VkPhysicalDevice physical_device, VkDevice device,
                        void *vertices, u32 vertex_size, u32 vertex_count,
                        void *indices, u32 index_size, u32 index_count)
{
    assert(vertices && indices);

    RenderMesh result = {};
    result.vertex_count = vertex_count;
    result.index_count = index_count;

    u32 total_vertex_size = vertex_size * vertex_count;
    u32 total_index_size = index_size * index_count;

    result.vertex_buffer = 
        vk_create_host_visible_coherent_buffer(physical_device, device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices, total_vertex_size);

    result.index_buffer = 
        vk_create_host_visible_coherent_buffer(physical_device, device, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indices, total_index_size);

    return result;
}
#endif

#if 0
internal void
vk_render_render_group(RenderGroup *render_group, RenderContext *render_context)
{
    Camera *camera = &render_group->camera;

    f32 width_over_height = (f32)render_context->window_width / (f32)render_context->window_height;
    m4 proj = projection(camera->focal_length, width_over_height, 0.1f, 10000.0f);
    m4 view = world_to_camera(camera->p, 
                            camera->initial_x_axis, camera->along_x, 
                            camera->initial_y_axis, camera->along_y, 
                            camera->initial_z_axis, camera->along_z);

    Uniform uniform = {};

    uniform.model = scale_m4(1.0f, 1.0f, 1.0f);
    uniform.proj_view = proj * camera_rhs_to_lhs(view);

    vk_update_uniform_buffer(&render_context->uniform_buffer, render_context->current_image_index, &uniform, sizeof(uniform));

    VkCommandBuffer *command_buffer = render_context->graphics_command_buffers + render_context->current_image_index;
    // TODO(joon) : fence needed here!
    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    CheckVkResult(vkBeginCommandBuffer(*command_buffer, &command_buffer_begin_info));

    VkClearValue clear_values[2] = {}; 
    clear_values[0].color = {0.0001f, 0.0001f, 0.0001f, 1};
    clear_values[1].depthStencil.depth = 1.0f;

    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = render_context->renderpass;
    renderPassBeginInfo.framebuffer = render_context->framebuffers[render_context->current_image_index];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent.width = (u32)render_context->surface_width;
    renderPassBeginInfo.renderArea.extent.height = (u32)render_context->surface_height;
    renderPassBeginInfo.clearValueCount = array_count(clear_values);
    renderPassBeginInfo.pClearValues = clear_values;
    vkCmdBeginRenderPass(*command_buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, render_context->pipeline);

    vk_set_viewport(*command_buffer, render_context->surface_width, render_context->surface_height, 0.0f, 1.0f);
    vk_set_scissor_rect(*command_buffer, render_context->surface_width, render_context->surface_height);

    VkWriteDescriptorSet write_descriptor_set[1] = {};

    VkDescriptorBufferInfo uniform_buffer_info = {};
    uniform_buffer_info.buffer = render_context->uniform_buffer.buffer;

    uniform_buffer_info.offset = render_context->current_image_index * render_context->uniform_buffer.element_size;
    uniform_buffer_info.range = render_context->uniform_buffer.element_size;

    write_descriptor_set[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write_descriptor_set[0].dstSet = 0;
    write_descriptor_set[0].dstBinding = 1;
    write_descriptor_set[0].descriptorCount = 1;
    write_descriptor_set[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write_descriptor_set[0].pBufferInfo = &uniform_buffer_info;

    vkCmdPushDescriptorSetKHR(*command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
            render_context->pipeline_layout, 0, array_count(write_descriptor_set), write_descriptor_set);

    for(u32 consumed = 0;
            consumed < render_group->render_memory.used;
            )
    {
        RenderEntryHeader *header = (RenderEntryHeader *)((u8 *)render_group->render_memory.base + consumed);
        consumed += sizeof(*header);

        switch(header->type)
        {
            case render_entry_type_cube:
            {
                RenderEntryCube *entry = (RenderEntryCube *)((u8 *)render_group->render_memory.base + consumed);
                consumed += sizeof(*entry);

                //RenderMesh(&render_context-> &helmetRenderMesh, *commandBuffer);

                //RenderMesh(&render_context-> &terrainRenderMesh, *commandBuffer);


            }break;

            case render_entry_type_line:
            {
                RenderEntryLine *entry = (RenderEntryLine *)((u8 *)render_group->render_memory.base + consumed);
                consumed += sizeof(*entry);

            }break;
        }
    }

    vkCmdEndRenderPass(*command_buffer);
    CheckVkResult(vkEndCommandBuffer(*command_buffer));

    u32 image_index = u32_max;
    VkResult acquire_next_image_result = vkAcquireNextImageKHR(render_context->device, render_context->swapchain, 
            UINT64_MAX, // TODO(joon) : Why not just use the next image, instead of waiting this image?
            render_context->ready_to_render_semaphores[render_context->current_image_index], // signaled when the application can use the image.
            0, // fence that will signaled when the application can use the image. unused for now.
            &image_index);

    if(acquire_next_image_result == VK_SUCCESS)
    {
        VkCommandBuffer *command_buffer = render_context->graphics_command_buffers + image_index;

#if 1

#endif

        // TODO(joon) : Check this value!
        VkPipelineStageFlags wait_flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkSubmitInfo queueSubmitInfo = {};
        queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        queueSubmitInfo.waitSemaphoreCount = 1;
        queueSubmitInfo.pWaitSemaphores = render_context->ready_to_render_semaphores + render_context->current_image_index;
        queueSubmitInfo.pWaitDstStageMask = &wait_flags;
        queueSubmitInfo.commandBufferCount = 1;
        queueSubmitInfo.pCommandBuffers = command_buffer;
        queueSubmitInfo.signalSemaphoreCount = 1;
        queueSubmitInfo.pSignalSemaphores = render_context->ready_to_present_semaphores + render_context->current_image_index;

        // TODO(joon) : Do we really need to specify fence here?
        CheckVkResult(vkQueueSubmit(render_context->graphics_queue, 1, &queueSubmitInfo, 0));

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = render_context->ready_to_present_semaphores + render_context->current_image_index;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = &render_context->swapchain;
        present_info.pImageIndices = &image_index;

        CheckVkResult(vkQueuePresentKHR(render_context->graphics_queue, &present_info));

        // NOTE(joon) : equivalent to calling vkQueueWaitIdle for all queues
        CheckVkResult(vkDeviceWaitIdle(render_context->device));
    }
    else if(acquire_next_image_result == VK_SUBOPTIMAL_KHR)
    {
        // TODO(joon) : screen resized!
    }
    else if(acquire_next_image_result == VK_NOT_READY)
    {
        // NOTE(joon) : Just use the next image
    }
    else
    {
        assert(acquire_next_image_result != VK_TIMEOUT);
        // NOTE : If the code gets inside here, one of these should have happened
        //VK_ERROR_OUT_OF_HOST_MEMORY
        //VK_ERROR_OUT_OF_DEVICE_MEMORY
        //VK_ERROR_DEVICE_LOST
        //VK_ERROR_OUT_OF_DATE_KHR
        //VK_ERROR_SURFACE_LOST_KHR
        //VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT
    }

    render_context->current_image_index = (render_context->current_image_index + 1)%render_context->swapchain_image_count;
}
#endif

    



