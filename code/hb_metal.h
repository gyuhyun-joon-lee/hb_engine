#ifndef HB_METAL_H
#define HB_METAL_H

// NOTE(joon) shared buffer between CPU and GPU, in a coherent way
struct MetalSharedBuffer
{
    u32 max_size;
    void *memory; // host memory

    id<MTLBuffer> buffer;
};

// NOTE(joon) shared buffer between CPU and GPU, but needs to be exclusively synchronized(flushed)
struct MetalManagedBuffer
{
    u32 max_size;
    void *memory; // host memory

    u32 used;
    id<MTLBuffer> buffer;
};

struct MetalRenderContext
{
    id<MTLDevice> device;
    MTKView *view;
    id<MTLCommandQueue> command_queue;
    id<MTLDepthStencilState> depth_state; // compare <=, write : enabled

    id<MTLRenderPipelineState> voxel_pipeline_state;
    id<MTLRenderPipelineState> cube_pipeline_state;
    id<MTLRenderPipelineState> line_pipeline_state;

    MetalManagedBuffer voxel_position_buffer;
    MetalManagedBuffer voxel_color_buffer;

    MetalManagedBuffer cube_inward_facing_index_buffer;
    MetalManagedBuffer cube_outward_facing_index_buffer;
};

#endif
