/*
 * Written by Gyuhyun Lee
 */

#ifndef HB_METAL_H
#define HB_METAL_H

// TODO(gh) newBufferWithBytesNoCopy looks very interesting, is it only possible in unified memory architectures?

// NOTE(joon) shared buffer between CPU and GPU, in a coherent way
struct MetalSharedBuffer
{
    u32 size;
    void *memory; // host memory

    id<MTLBuffer> buffer;
};

// NOTE(joon) shared buffer between CPU and GPU, but needs to be exclusively synchronized(flushed)
struct MetalManagedBuffer
{
    id<MTLBuffer> buffer;

    u32 size;
    void *memory; // host memory
};

struct MetalPrivateBuffer
{
    id<MTLBuffer> buffer;
    u32 size;

    // CPU does not have access to Private buffer, so no pointer to system memory
};

// TODO(gh) maybe this is a good idea?
struct MetalTexture2D
{
    id<MTLTexture> texture;
    i32 width;
    i32 height;

    // TODO(gh) maybe also store usage and storage mode?
};

struct MetalTexture3D
{
    id<MTLTexture> texture;
    i32 width;
    i32 height;
    i32 depth;
};

struct MetalTimestamp
{
    u64 cpu;
    u64 gpu;
};

struct MetalRenderContext
{
    id<MTLDevice> device;
    MTKView *view;
    id<MTLCommandQueue> command_queue;

    id<MTLDepthStencilState> depth_state; // compare <=, write : enabled
    id<MTLDepthStencilState> disabled_depth_state; // write : disabled

    // Samplers
    // Samplers defined inside the shader don't have full capability,
    // which is why we are passing these samplers that are made exeternally.
    id<MTLSamplerState> shadowmap_sampler;

    // Render Pipelines
    id<MTLRenderPipelineState> directional_light_shadowmap_pipeline;
    // NOTE(gh) Those that are marked as 'singlepass' will be sharing 
    id<MTLRenderPipelineState> singlepass_cube_pipeline;
    id<MTLRenderPipelineState> deferred_pipeline;
    id<MTLRenderPipelineState> instanced_grass_render_pipeline;

    // Forward Pipelines
    id<MTLRenderPipelineState> forward_line_pipeline;
    id<MTLRenderPipelineState> screen_space_triangle_pipeline;
    id<MTLRenderPipelineState> forward_render_perlin_noise_grid_pipeline;
    id<MTLRenderPipelineState> forward_render_game_camera_frustum_pipeline;

    // Compute Pipelines
    id<MTLComputePipelineState> add_compute_pipeline;
    id<MTLComputePipelineState> initialize_grass_counts_pipeline;

    id<MTLComputePipelineState> fill_grass_instance_data_pipeline;
    id<MTLComputePipelineState> encode_instanced_grass_render_commands_pipeline;

    // Mesh Pipelines(although the type is same as render pipeline)
    id<MTLRenderPipelineState> grass_mesh_render_pipeline;

    id<MTLRenderPipelineState> forward_render_font_pipeline;
    
    // Renderpasses
    MTLRenderPassDescriptor *directional_light_shadowmap_renderpass;
    // TODO(gh) There might be more effeicient way of doing this, 
    // so think about tiled forward rendering or clustered forward rendering
    // when we 'really' need it. 
    // TODO(gh) Also, we can combine this with the singlepass pretty easily
    MTLRenderPassDescriptor *forward_renderpass;

    // TODO(gh) do we really need this clear pass?
    MTLRenderPassDescriptor *clear_g_buffer_renderpass; 
    MTLRenderPassDescriptor *g_buffer_renderpass; 
    MTLRenderPassDescriptor *deferred_renderpass; 

    // Fences
    // Fence to detect whether the deferred rendering has been finished before doing the foward rendering
    id<MTLFence> forward_render_fence;
    id<MTLFence> grass_double_buffer_fence[2];

    // Textures
    MetalTexture2D g_buffer_position_texture;
    MetalTexture2D g_buffer_normal_texture;
    MetalTexture2D g_buffer_color_texture;
    MetalTexture2D g_buffer_depth_texture;
    MetalTexture2D directional_light_shadowmap_depth_texture;

    // Buffers
    u32 next_grass_double_buffer_index;
    id<MTLIndirectCommandBuffer> icb[2]; 
    MetalSharedBuffer icb_argument_buffer[2];
    MetalSharedBuffer transient_buffer; // will be passed on to the game code

    MetalSharedBuffer giant_buffer; // will be passed on to the game code

    MetalSharedBuffer grass_count_buffer;// This one gets initialized to 0 at the start of the rendering loop, and then accumulates
    MetalSharedBuffer grass_index_buffer;

    // Timestamps
    // MetalTimestamp shadowmap_rendering_start_timestamp;
    // MetalTimestamp shadowmap_rendering_end_timestamp;

    MetalTimestamp grass_rendering_start_timestamp;
    MetalTimestamp grass_rendering_end_timestamp;
};

#endif
