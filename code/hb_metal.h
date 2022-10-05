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
    // single renderpass & color attachments will be sharing single renderpass & color attachments
    id<MTLRenderPipelineState> singlepass_cube_pipeline;
    id<MTLRenderPipelineState> singlepass_deferred_lighting_pipeline;

    // Forward Pipelines
    id<MTLRenderPipelineState> forward_line_pipeline;
    id<MTLRenderPipelineState> screen_space_triangle_pipeline;
    id<MTLRenderPipelineState> forward_show_perlin_noise_grid_pipeline;

    // Compute Pipelines
    id<MTLComputePipelineState> add_compute_pipeline;

    // Mesh Pipelines(although the type is same as render pipeline)
    id<MTLRenderPipelineState> grass_mesh_render_pipeline;
    
    // Renderpasses
    MTLRenderPassDescriptor *directional_light_shadowmap_renderpass;
    // TODO(gh) There might be more effeicient way of doing this, 
    // so think about tiled forward rendering or clustered forward rendering
    // when we 'really' need it. 
    // TODO(gh) Also, we can combine this with the singlepass pretty easily
    MTLRenderPassDescriptor *forward_renderpass;

    // NOTE(gh) G buffer generation + lighting renderpass
    // TODO(gh) One downside of this single pass is that we don't have access to depth buffer (wait, what?)
    MTLRenderPassDescriptor *single_renderpass; 

    // Fences
    // Fence to detect whether the deferred rendering has been finished beforedoing the foward rendering
    id<MTLFence> forwardRenderFence;

    // Textures
    id<MTLTexture> g_buffer_position_texture;
    id<MTLTexture> g_buffer_normal_texture;
    id<MTLTexture> g_buffer_color_texture;
    id<MTLTexture> g_buffer_depth_texture;

    i32 directional_light_shadowmap_depth_texture_width;
    i32 directional_light_shadowmap_depth_texture_height;
    id<MTLTexture> directional_light_shadowmap_depth_texture;

    // Buffers
    MetalSharedBuffer combined_vertex_buffer;
    MetalSharedBuffer combined_index_buffer;

    MetalSharedBuffer voxel_position_buffer;
    MetalSharedBuffer voxel_color_buffer;

    MetalSharedBuffer cube_inward_facing_index_buffer;
    MetalSharedBuffer cube_outward_facing_index_buffer;

    MetalSharedBuffer random_grass_hash_buffer;
    MetalSharedBuffer perlin_value_buffer;
    MetalSharedBuffer floor_z_buffer;
};

#endif
