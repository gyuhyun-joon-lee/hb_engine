#include "hb_metal.h"

#define check_ns_error(error)\
{\
    if(error)\
    {\
        printf("domain: %s code: %ld, ", [error.domain UTF8String], (long)error.code);\
        printf("%s\n", [[error localizedDescription] UTF8String]); \
        assert(0);\
    }\
}\

internal b32
metal_does_support_gpu_family(id<MTLDevice> device, MTLGPUFamily gpu_family)
{
    b32 result = [device supportsFamily : gpu_family];
    return result;
}

// NOTE(gh) orthogonal projection matrix for (-1, -1, 0) to (1, 1, 1) NDC like Metal
inline m4x4
orthogonal_projection(f32 n, f32 f, f32 width, f32 width_over_height)
{
    f32 height = width / width_over_height;

    m4x4 result = {};

    // TODO(gh) Even though the resulting coordinates should be same, 
    // it seems like Metal requires the w part of the homogeneous coordinate to be positive.
    // Maybe that's how they are doing the frustum culling..?
    result.rows[0] = V4(1/(width/2), 0, 0, 0);
    result.rows[1] = V4(0, 1/(height/2), 0, 0);
    result.rows[2] = V4(0, 0, 1/(n-f), n/(n-f)); // X and Y value does not effect the z value
    result.rows[3] = V4(0, 0, 0, 1); // orthogonal projection doesn't need homogeneous coordinates

    return result;
}

// NOTE(gh) persepctive projection matrix for (-1, -1, 0) to (1, 1, 1) NDC like Metal
/*
    Little tip in how we get the persepctive projection matrix
    Think as 2D plane(x and z OR y and z), use triangle similarity to get projected Xp and Yp
    For Z, as x and y don't have any effect on Z, we can say (A * Ze + b) / -Ze = Zp (diving by -Ze because homogeneous coords)

    Based on what NDC we are using, -n should produce 0 or -1 value, while -f should produce 1.
*/
// TODO(gh) Make this to be dependent to the FOV, not the actual width
// because it becomes irrelevant when we change the near plane. 
inline m4x4
perspective_projection(f32 fov, f32 n, f32 f, f32 width_over_height)
{
    assert(fov < 180);

    f32 half_near_plane_width = n*tanf(0.5f*fov)*0.5f;
    f32 half_near_plane_height = half_near_plane_width / width_over_height;

    m4x4 result = {};

    // TODO(gh) Even though the resulting coordinates should be same, 
    // it seems like Metal requires the w part of the homogeneous coordinate to be positive.
    // Maybe that's how they are doing the frustum culling..?
    result.rows[0] = V4(n / half_near_plane_width, 0, 0, 0);
    result.rows[1] = V4(0, n / half_near_plane_height, 0, 0);
    result.rows[2] = V4(0, 0, f/(n-f), (n*f)/(n-f)); // X and Y value does not effect the z value
    result.rows[3] = V4(0, 0, -1, 0); // As Xp and Yp are dependant to Z value, this is the only way to divide Xp and Yp by -Ze

    return result;
}

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
metal_set_fragment_sampler(id<MTLRenderCommandEncoder> render_encoder, id<MTLSamplerState> sampler, u32 index)
{
    [render_encoder setFragmentSamplerState : sampler
                                atIndex : index];
}

internal void
metal_set_depth_bias(id<MTLRenderCommandEncoder> render_encoder, f32 depth_bias, f32 slope_scale, f32 clamp)
{
    [render_encoder setDepthBias:depth_bias slopeScale:slope_scale clamp:clamp];
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

    id<MTLFunction> vertex_shader = 0;
    id<MTLFunction> fragment_shader = 0;
    if(vertex_shader_name)
    {
        vertex_shader = [shader_library newFunctionWithName:[NSString stringWithUTF8String:vertex_shader_name]];
    }

    if(fragment_shader_name)
    {
        fragment_shader = [shader_library newFunctionWithName:[NSString stringWithUTF8String:fragment_shader_name]];
    }

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

internal MTLRenderPassDescriptor *
metal_make_renderpass(MTLLoadAction *load_actions, u32 load_action_count,
                      MTLStoreAction *store_actions, u32 store_action_count,
                      id<MTLTexture> *textures, u32 texture_count,
                      v4 *clear_colors, u32 clear_color_count,
                      MTLLoadAction depth_load_action, MTLStoreAction depth_store_action, 
                      id<MTLTexture> depth_texture,
                      f32 depth_clear_value)
{
    assert(load_action_count == store_action_count); 
    assert(load_action_count == texture_count); 
    assert(load_action_count == clear_color_count);

    MTLRenderPassDescriptor *result = [MTLRenderPassDescriptor new];

    for(u32 color_attachment_index = 0;
            color_attachment_index < texture_count;
            ++color_attachment_index)
    {
        result.colorAttachments[color_attachment_index].loadAction = load_actions[color_attachment_index]; // Will DONTCARE work here?
        result.colorAttachments[color_attachment_index].storeAction = store_actions[color_attachment_index];

        // NOTE(gh) Both textures and clear_colors are not essential
        if(textures)
        {
            result.colorAttachments[color_attachment_index].texture = textures[color_attachment_index];
        }

        if(clear_colors)
        {
            result.colorAttachments[color_attachment_index].clearColor = {clear_colors[color_attachment_index].r,
                                                                          clear_colors[color_attachment_index].g,
                                                                          clear_colors[color_attachment_index].b,
                                                                          clear_colors[color_attachment_index].a};
        }
    }

    result.depthAttachment.loadAction = depth_load_action;
    result.depthAttachment.storeAction = depth_store_action;
    result.depthAttachment.texture = depth_texture;
    result.depthAttachment.clearDepth = depth_clear_value;

    return result;
}

internal id<MTLSamplerState> 
metal_make_sampler(id<MTLDevice> device, b32 normalized_coordinates, MTLSamplerAddressMode address_mode, 
                   MTLSamplerMinMagFilter min_mag_filter, MTLSamplerMipFilter mip_filter, MTLCompareFunction compare_function)
{
    MTLSamplerDescriptor *sampler_descriptor = [[MTLSamplerDescriptor alloc] init];
    sampler_descriptor.normalizedCoordinates = normalized_coordinates;
    sampler_descriptor.sAddressMode = address_mode; // width coordinate
    sampler_descriptor.tAddressMode = address_mode; // height coordinate
    sampler_descriptor.rAddressMode = address_mode; // depth coordinate
    sampler_descriptor.minFilter = min_mag_filter;
    sampler_descriptor.magFilter = min_mag_filter;
    sampler_descriptor.mipFilter = mip_filter;
    sampler_descriptor.maxAnisotropy = 1;
    sampler_descriptor.compareFunction = compare_function;

    id<MTLSamplerState> result = [device newSamplerStateWithDescriptor : sampler_descriptor];

    [sampler_descriptor release];

    return result;
}


        








