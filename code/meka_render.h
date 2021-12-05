#ifndef MEKA_RENDER_H
#define MEKA_RENDER_H

#if 1
// NOTE(joon) : This forces us to copy these datas again to the actual 'usable' vertex buffer,
// but even with more than 100000 vertices, the size of it isn't too big 
// TODO(joon) : loaded_raw_mesh is independent from any graphics api, put this in other places? or maybe not...
struct raw_mesh
{
    v3 *positions;
    u32 position_count;

    v3 *normals;
    u32 normal_count;

    v2 *texcoords;
    u32 texcoord_count;

    u32 *indices;
    u32 index_count;

    u32 *normal_indices;
    u32 normal_index_count;

    u32 *texcoord_indices;
    u32 texcoord_index_count;
};

// mesh that is ready in a form thatto be consumed by the renderer
struct render_mesh
{
#if MEKA_VULKAN
    vk_host_visible_coherent_buffer vertexBuffer;
    vk_host_visible_coherent_buffer indexBuffer;
#elif MEKA_METAL
    // NOTE(joon): These are only accessible by the GPU
    id<MTLBuffer> vertex_buffer;
    id<MTLBuffer> index_buffer;
#endif

    unsigned index_count;
};

struct camera
{
    v3 p;
    union
    {
        struct
        {
            r32 along_x;
            r32 along_y;
            r32 along_z;
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

#if  0
struct uniform_buffer
{
    // NOTE(joon) : mat4 should be 16 bytes aligned
    alignas(16) m4 model;
    alignas(16) m4 projView;
    alignas(16) v3 lightP;
};
#endif
#endif

#endif
