#include "hb_metal.h"

#define check_ns_error(error)\
{\
    if(error)\
    {\
        printf("check_metal_error failed inside the domain: %s code: %ld\n", [error.domain UTF8String], (long)error.code);\
        assert(0);\
    }\
}\

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
    if(buffer->used)
    {
        NSRange range = NSMakeRange(0, buffer->used);
        [buffer->buffer didModifyRange:range];
        buffer->used = 0;
    }
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
metal_set_vertex_texture(id<MTLRenderCommandEncoder> render_encoder, id<MTLTexture> texture, u32 index)
{
    [render_encoder setVertexTexture : texture
                    atIndex : index];
}

internal void
metal_set_fragment_texture(id<MTLRenderCommandEncoder> render_encoder, id<MTLTexture> texture, u32 index)
{
    [render_encoder setFragmentTexture : texture
                             atIndex:index];
}

internal void
metal_draw_non_indexed(id<MTLRenderCommandEncoder> render_encoder, MTLPrimitiveType primitive_type, 
                        u32 vertex_start, u32 vertex_count)
{
    // NOTE(gh) vertex_id will start from vertex_start to vertex_start + vertex_count - 1
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

internal id<MTLRenderPipelineState>
metal_make_pipeline(id<MTLDevice> device,
                    const char *pipeline_name, const char *vertex_shader_name, const char *fragment_shader_name,
                    id<MTLLibrary> shader_library,
                    MTLPrimitiveTopologyClass topology, 
                    MTLPixelFormat *pixel_formats, u32 pixel_format_count, 
                    MTLColorWriteMask *masks, u32 mask_count,
                    MTLPixelFormat depth_pixel_format)
{
    assert(pixel_format_count == mask_count);

    id<MTLFunction> vertex_shader = [shader_library newFunctionWithName:[NSString stringWithUTF8String:vertex_shader_name]];
    id<MTLFunction> fragment_shader = [shader_library newFunctionWithName:[NSString stringWithUTF8String:fragment_shader_name]];

    MTLRenderPipelineDescriptor *pipeline_descriptor = [MTLRenderPipelineDescriptor new];
    pipeline_descriptor.label = [NSString stringWithUTF8String:pipeline_name];
    pipeline_descriptor.vertexFunction = vertex_shader;
    pipeline_descriptor.fragmentFunction = fragment_shader;
    pipeline_descriptor.sampleCount = 1;
    pipeline_descriptor.rasterSampleCount = pipeline_descriptor.sampleCount;
    pipeline_descriptor.rasterizationEnabled = true;
    pipeline_descriptor.inputPrimitiveTopology = topology;

    for(u32 color_attachment_index = 0;
            color_attachment_index < pixel_format_count;
            ++color_attachment_index)
    {
        pipeline_descriptor.colorAttachments[color_attachment_index].pixelFormat = pixel_formats[color_attachment_index];
        pipeline_descriptor.colorAttachments[color_attachment_index].writeMask = masks[color_attachment_index];
    }


    pipeline_descriptor.depthAttachmentPixelFormat = depth_pixel_format;

    NSError *error;
    id<MTLRenderPipelineState> result = [device newRenderPipelineStateWithDescriptor:pipeline_descriptor
                                                                error:&error];
    check_ns_error(error);

    [pipeline_descriptor release];

    return result;
}

internal id<MTLTexture>
metal_make_texture_2D(id<MTLDevice> device, MTLPixelFormat pixel_format, i32 width, i32 height, 
                  MTLTextureType type, MTLTextureUsage usage, MTLStorageMode storage_mode)
{
    MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixel_format
                                                                   width:width
                                                                  height:height
                                                               mipmapped:NO];
    descriptor.textureType = type;
    descriptor.usage |= usage;
    descriptor.storageMode =  storage_mode;

    id<MTLTexture> result  = [device newTextureWithDescriptor:descriptor];
    [descriptor release];

    return result;
}

internal id<MTLDepthStencilState>
metal_make_depth_state(id<MTLDevice> device, MTLCompareFunction compare_function, b32 should_enable_write)
{
    /*
       MTLCompareFunction
        never : A new value never passes the comparison test.
        always : A new value always passes the comparison test.
    */
    MTLDepthStencilDescriptor *depth_descriptor = [MTLDepthStencilDescriptor new];
    depth_descriptor.depthCompareFunction = compare_function;
    depth_descriptor.depthWriteEnabled = should_enable_write;

    id<MTLDepthStencilState> result = [device newDepthStencilStateWithDescriptor:depth_descriptor];
    [depth_descriptor release];

    return result;
}

        








