#include "meka_metal.h"

internal void
metal_set_viewport(id<MTLRenderCommandEncoder> render_encoder, f32 x, f32 y, f32 width, f32 height, f32 near, f32 far)
{
    MTLViewport viewport = {};

    viewport.originX = (f32)x;
    viewport.originY = (f32)y;
    viewport.width = (f32)width;
    viewport.height = (f32)height;
    viewport.znear = (f32)near;
    viewport.zfar = (f32)far;

    [render_encoder setViewport: viewport];
}

internal void
metal_set_scissor_rect(id<MTLRenderCommandEncoder> render_encoder, u32 x, u32 y, u32 width, u32 height)
{
    MTLScissorRect scissor_rect = {};

    scissor_rect.x = x;
    scissor_rect.y = y;
    scissor_rect.width = width;
    scissor_rect.height = height;

    [render_encoder setScissorRect: scissor_rect];
}

// NOTE(joon) creates a shared memory between CPU and GPU
// TODO(joon) Do we want the buffer to be more than 4GB?
internal MetalSharedBuffer
metal_create_shared_buffer(id<MTLDevice> device, u32 max_size)
{
    MetalSharedBuffer result = {};
    result.buffer = [device 
                    newBufferWithLength:max_size
                    options:MTLResourceStorageModeShared];

    result.memory = [result.buffer contents];
    result.max_size = max_size;

    return result;
}

internal void
metal_write_shared_buffer(MetalSharedBuffer *buffer, void *source, u32 source_size)
{
    assert(source_size <= buffer->max_size);
    memcpy(buffer->memory, source, source_size);
}

internal MetalManagedBuffer
metal_create_managed_buffer(id<MTLDevice> device, u32 max_size)
{
    MetalManagedBuffer result = {};
    result.buffer = [device 
                    newBufferWithLength:max_size
                    options:MTLResourceStorageModeManaged];

    result.memory = [result.buffer contents];
    result.max_size = max_size;

    return result;
}

internal void
metal_append_to_managed_buffer(MetalManagedBuffer *buffer, void *source, u32 source_size)
{
    memcpy((u8 *)buffer->memory + buffer->used, source, source_size);
    buffer->used += source_size;

    assert(buffer->used <= buffer->max_size);
}

internal void
metal_flush_managed_buffer(MetalManagedBuffer *buffer)
{
    assert(buffer->used != 0);
    NSRange range = NSMakeRange(0, buffer->used);

    [buffer->buffer didModifyRange:range];

    buffer->used = 0;

}

// NOTE(joon) wrapping functions
// TODO(joon) might do some interesting things by inserting something here...(i.e renderdoc?)
internal void
metal_set_pipeline(id<MTLRenderCommandEncoder> render_encoder, id<MTLRenderPipelineState> pipeline)
{
    [render_encoder setRenderPipelineState : pipeline];
}

internal void
metal_set_triangle_fill_mode(id<MTLRenderCommandEncoder> render_encoder, MTLTriangleFillMode fill_mode)
{
    [render_encoder setTriangleFillMode : fill_mode];
}

internal void
metal_set_front_facing_winding(id<MTLRenderCommandEncoder> render_encoder, MTLWinding winding)
{
    [render_encoder setFrontFacingWinding :winding];
}

internal void
metal_set_cull_mode(id<MTLRenderCommandEncoder> render_encoder, MTLCullMode cull_mode)
{
    [render_encoder setCullMode :cull_mode];
}

internal void
metal_set_detph_stencil_state(id<MTLRenderCommandEncoder> render_encoder, id<MTLDepthStencilState> depth_stencil_state)
{
    [render_encoder setDepthStencilState: depth_stencil_state];
}

internal void
metal_set_vertex_bytes(id<MTLRenderCommandEncoder> render_encoder, void *data, u32 size, u32 index)
{
    [render_encoder setVertexBytes: data 
                    length: size 
                    atIndex: index]; 
}

internal void
metal_set_vertex_buffer(id<MTLRenderCommandEncoder> render_encoder, id<MTLBuffer> buffer, u32 offset, u32 index)
{
    [render_encoder setVertexBuffer:buffer
                    offset:offset
                    atIndex:index];
}

internal void
metal_draw_non_indexed(id<MTLRenderCommandEncoder> render_encoder, MTLPrimitiveType primitive_type, 
                        u32 vertex_start, u32 vertex_count)
{
    [render_encoder drawPrimitives:primitive_type 
                    vertexStart:vertex_start
                    vertexCount:vertex_count];
}

internal void
metal_draw_non_indexed_instances(id<MTLRenderCommandEncoder> render_encoder, MTLPrimitiveType primitive_type,
                                u32 vertex_start, u32 vertex_count, u32 instance_start, u32 instance_count)
{
    [render_encoder drawPrimitives: primitive_type
                    vertexStart: vertex_start
                    vertexCount: vertex_count 
                    instanceCount: instance_count 
                    baseInstance: instance_start];
}

internal void
metal_draw_indexed_instances(id<MTLRenderCommandEncoder> render_encoder, MTLPrimitiveType primitive_type,
                            id<MTLBuffer> index_buffer, u32 index_count, u32 instance_count)
{
    [render_encoder drawIndexedPrimitives:primitive_type
                    indexCount:index_count
                    indexType:MTLIndexTypeUInt32
                    indexBuffer:index_buffer 
                    indexBufferOffset:0 
                    instanceCount:instance_count];
}

internal void
metal_end_encoding(id<MTLRenderCommandEncoder> render_encoder)
{
    [render_encoder endEncoding];
}

internal void
metal_present_drawable(id<MTLCommandBuffer> command_buffer, MTKView *view)
{
    if(view.currentDrawable)
    {
        [command_buffer presentDrawable: view.currentDrawable];
    }
    else
    {
        invalid_code_path;
    }
}

internal void
metal_commit_command_buffer(id<MTLCommandBuffer> command_buffer, MTKView *view)
{
    [command_buffer commit];
}

        








