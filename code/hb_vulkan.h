/*
 * Written by Gyuhyun Lee
 */

#ifndef MEKA_VULKAN_H
#define MEKA_VULKAN_H

#define VK_NO_PROTOTYPES 
#define VK_USE_PLATFORM_METAL_EXT
#include "../external/vulkan/macos/include/vulkan/vulkan.h"

struct vk_queue_families
{
    u32 graphicsQueueFamilyIndex;
    u32 graphicsQueueCount;
    u32 presentQueueFamilyIndex;
    u32 presentQueueCount;
    u32 transferQueueFamilyIndex;
    u32 transferQueueCount;
};

struct vk_host_visible_coherent_buffer
{
    VkBuffer buffer;
    u32 size;
    VkDeviceMemory device_memory;

    void *memory; // memory that we can directly write into
};

// NOTE(joon) similiar to openGL, vulkan requires the uniform buffer to have certain alignment(normally 256)
// so this is pretty similar to the host visible coherent buffer, except the fact that we are keep
// tracking the size of the individual elements
struct vk_uniform_buffer
{
    VkBuffer buffer;

    void *memory; // memory that we can directly write into
    VkDeviceMemory device_memory;
    
    u32 element_size;
    u32 element_count;
};

#endif
