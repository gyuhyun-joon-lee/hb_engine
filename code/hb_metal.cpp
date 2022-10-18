/*
 * Written by Gyuhyun Lee
 */

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
metal_does_support_gpu_timestamp(id<MTLDevice> device)
{
    b32 result = false;
    NSArray<id<MTLCounterSet>> *counter_sets = device.counterSets;

    id< MTLCounterSet > timestamp_counter_set = 0;
    
    // Look at this nonsense...
    for ( id< MTLCounterSet > counter_set in counter_sets )
    {
        NSString* counter_set_name = counter_set.name;
        if ( [counter_set_name caseInsensitiveCompare:MTLCommonCounterSetTimestamp] == NSOrderedSame )
        {
            timestamp_counter_set = counter_set;
            break;
        }

    }

    NSArray<id<MTLCounter>>* counters_in_set = timestamp_counter_set.counters;
    for ( id< MTLCounter > counter in counters_in_set )
    {
        if ( [counter.name caseInsensitiveCompare:MTLCommonCounterTimestamp] == NSOrderedSame )
        {
            result = true;
            break;
        }
    }

    return result;
}

// NOTE(gh) This function may trap to the kernel to read the GPU clock, so it's not free.
internal void
metal_get_timestamp(MetalTimestamp *timestamp, id<MTLCommandBuffer> command_buffer, id<MTLDevice> device)
{
    // MTLTimestamp == u64

    // NOTE(gh) addCompletedHandler registers a block of code that Metal calls immediately 
    // after the GPU finishes executing the commands in the command buffer.
    [command_buffer addCompletedHandler:^(id<MTLCommandBuffer> _Nonnull handler) 
    {
        // Save the timestamps as soon as the GPU finishes executing the command buffer.
        [device sampleTimestamps:(MTLTimestamp *)&timestamp->cpu 
                    gpuTimestamp:(MTLTimestamp *)&timestamp->gpu];
    }
    ];
}

internal b32
metal_does_support_gpu_family(id<MTLDevice> device, MTLGPUFamily gpu_family)
{
    b32 result = [device supportsFamily : gpu_family];
    return result;
}

internal MTLSize
metal_make_mtl_size(v3u value)
{
    MTLSize result = MTLSizeMake(value.x, value.y, value.z);
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

    -n should produce 0 or -1 value(based on what NDC system we use), 
    while -f should produce 1.
*/
inline m4x4
perspective_projection(f32 fov, f32 n, f32 f, f32 width_over_height)
{
    assert(fov < 180);

    f32 half_near_plane_width = n*tanf(0.5f*fov)*0.5f;
    f32 half_near_plane_height = half_near_plane_width / width_over_height;

    m4x4 result = {};

    // TODO(gh) Even though the resulting coordinates should be same, 
    // it seems like in Metal w value should be positive.
    // Maybe that's how they are doing the frustum culling..?
    result.rows[0] = V4(n / half_near_plane_width, 0, 0, 0);
    result.rows[1] = V4(0, n / half_near_plane_height, 0, 0);
    result.rows[2] = V4(0, 0, f/(n-f), (n*f)/(n-f)); // X and Y value does not effect the z value
    result.rows[3] = V4(0, 0, -1, 0); // As Xp and Yp are dependant to Z value, this is the only way to divide Xp and Yp by -Ze

    return result;
}

inline void
metal_signal_fence_after(id<MTLRenderCommandEncoder> encoder, id<MTLFence> fence, MTLRenderStages after_render_stage)
{
    // Make GPU signal the fence after executing the specified render stage
    [encoder updateFence : 
            fence
            afterStages:after_render_stage];
}

inline void
metal_wait_for_fence(id<MTLRenderCommandEncoder> encoder, id<MTLFence> fence, MTLRenderStages before_render_stage)
{
    // Make GPU wait for the fence before executing the specified render stage
    [encoder waitForFence : 
            fence
            beforeStages : before_render_stage];
}

inline void
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

inline void
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
inline MetalSharedBuffer
metal_make_shared_buffer(id<MTLDevice> device, u64 max_size)
{
    MetalSharedBuffer result = {};
    result.buffer = [device 
                    newBufferWithLength:max_size
                    options:MTLResourceStorageModeShared];

    result.memory = [result.buffer contents];
    result.size = max_size;

    return result;
}

inline void
metal_write_shared_buffer(MetalSharedBuffer *buffer, void *source, u64 source_size)
{
    assert(source_size <= buffer->size);
    memcpy(buffer->memory, source, source_size);
}

inline MetalSharedBuffer
metal_make_shared_buffer(id<MTLDevice> device, void *source, u64 max_size)
{
    MetalSharedBuffer result = metal_make_shared_buffer(device, max_size);
    metal_write_shared_buffer(&result, source, max_size);

    return result;
}

inline MetalManagedBuffer
metal_make_managed_buffer(id<MTLDevice> device, u64 size)
{
    MetalManagedBuffer result = {};
    result.buffer = [device 
                    newBufferWithLength:size
                    options:MTLResourceStorageModeManaged];

    result.memory = [result.buffer contents];
    result.size = size;

    return result;
}

inline void
metal_update_managed_buffer(MetalManagedBuffer *buffer, u64 start, u64 size_to_update)
{
    assert(size_to_update <= buffer->size);
    NSRange range = NSMakeRange(start, size_to_update);
    [buffer->buffer didModifyRange:range];
}

inline MetalManagedBuffer
metal_make_managed_buffer(id<MTLDevice> device, void *source, u64 size)
{
    MetalManagedBuffer result = metal_make_managed_buffer(device, size);
    memcpy(result.memory, source, size);
    metal_update_managed_buffer(&result, 0, size);

    return result;
}

// NOTE(gh) no bound checking here
inline MetalPrivateBuffer
metal_make_private_buffer(id<MTLDevice> device, void *source, u64 size)
{
    MetalPrivateBuffer result = {};

    result.buffer = [device newBufferWithBytes:
                            source
                            length : size 
                            options : MTLResourceStorageModePrivate];
    result.size = size;

    return result;
}

// NOTE(joon) wrapping functions
// TODO(joon) might do some interesting things by inserting something here...(i.e renderdoc?)
inline void
metal_set_render_pipeline(id<MTLRenderCommandEncoder> render_encoder, id<MTLRenderPipelineState> pipeline)
{
    [render_encoder setRenderPipelineState : pipeline];
}

inline void
metal_set_triangle_fill_mode(id<MTLRenderCommandEncoder> render_encoder, MTLTriangleFillMode fill_mode)
{
    [render_encoder setTriangleFillMode : fill_mode];
}

inline void
metal_set_front_facing_winding(id<MTLRenderCommandEncoder> render_encoder, MTLWinding winding)
{
    [render_encoder setFrontFacingWinding :winding];
}

inline void
metal_set_cull_mode(id<MTLRenderCommandEncoder> render_encoder, MTLCullMode cull_mode)
{
    [render_encoder setCullMode :cull_mode];
}

inline void
metal_set_depth_stencil_state(id<MTLRenderCommandEncoder> render_encoder, id<MTLDepthStencilState> depth_stencil_state)
{
    [render_encoder setDepthStencilState: depth_stencil_state];
}

inline void
metal_set_vertex_bytes(id<MTLRenderCommandEncoder> render_encoder, void *data, u64 size, u64 index)
{
    [render_encoder setVertexBytes: data 
                    length: size 
                    atIndex: index]; 
}

inline void
metal_set_vertex_buffer(id<MTLRenderCommandEncoder> render_encoder, id<MTLBuffer> buffer, u64 offset, u64 index)
{
    [render_encoder setVertexBuffer:buffer
                    offset:offset
                    atIndex:index];
}

inline void
metal_set_vertex_texture(id<MTLRenderCommandEncoder> render_encoder, id<MTLTexture> texture, u64 index)
{
    [render_encoder setVertexTexture : texture
                    atIndex : index];
}

inline void
metal_set_fragment_texture(id<MTLRenderCommandEncoder> render_encoder, id<MTLTexture> texture, u64 index)
{
    [render_encoder setFragmentTexture : texture
                             atIndex:index];
}

inline void
metal_set_fragment_sampler(id<MTLRenderCommandEncoder> render_encoder, id<MTLSamplerState> sampler, u64 index)
{
    [render_encoder setFragmentSamplerState : sampler
                                atIndex : index];
}

inline void
metal_set_depth_bias(id<MTLRenderCommandEncoder> render_encoder, f32 depth_bias, f32 slope_scale, f32 clamp)
{
    [render_encoder setDepthBias:depth_bias slopeScale:slope_scale clamp:clamp];
}

inline void
metal_draw_non_indexed(id<MTLRenderCommandEncoder> render_encoder, MTLPrimitiveType primitive_type, 
                        u64 vertex_start, u64 vertex_count)
{
    // NOTE(gh) vertex_id will start from vertex_start to vertex_start + vertex_count - 1
    [render_encoder drawPrimitives:primitive_type 
                    vertexStart:vertex_start
                    vertexCount:vertex_count];
}

inline void
metal_draw_non_indexed_instances(id<MTLRenderCommandEncoder> render_encoder, MTLPrimitiveType primitive_type,
                                u64 vertex_start, u64 vertex_count, u64 instance_start, u64 instance_count)
{
    [render_encoder drawPrimitives: primitive_type
                    vertexStart: vertex_start
                    vertexCount: vertex_count 
                    instanceCount: instance_count 
                    baseInstance: instance_start];
}

inline void
metal_draw_indexed(id<MTLRenderCommandEncoder> render_encoder, MTLPrimitiveType primitive_type, 
                    id<MTLBuffer> index_buffer, u64 index_offset, u64 index_count)
{
    [render_encoder drawIndexedPrimitives : primitive_type
                   indexCount : index_count 
                    indexType : MTLIndexTypeUInt32 
                  indexBuffer : index_buffer 
            indexBufferOffset : index_offset];
}

inline void
metal_draw_indexed_instances(id<MTLRenderCommandEncoder> render_encoder, MTLPrimitiveType primitive_type,
                            id<MTLBuffer> index_buffer, u64 index_count, u64 instance_count)
{
    [render_encoder drawIndexedPrimitives:primitive_type
                    indexCount:index_count
                    indexType:MTLIndexTypeUInt32
                    indexBuffer:index_buffer 
                    indexBufferOffset:0 
                    instanceCount:instance_count];
}

inline void
metal_end_encoding(id<MTLRenderCommandEncoder> render_encoder)
{
    [render_encoder endEncoding];
}

inline void
metal_commit_command_buffer(id<MTLCommandBuffer> command_buffer)
{
    // NOTE(gh) also enqueues the command buffer into command queue
    [command_buffer commit];
}

inline void
metal_wait_until_command_buffer_completed(id<MTLCommandBuffer> command_buffer)
{
    [command_buffer waitUntilCompleted];
}

inline id<MTLRenderPipelineState>
metal_make_render_pipeline(id<MTLDevice> device,
                    const char *pipeline_name, const char *vertex_shader_name, const char *fragment_shader_name,
                    id<MTLLibrary> shader_library,
                    MTLPrimitiveTopologyClass topology, 
                    MTLPixelFormat *pixel_formats, u32 pixel_format_count, 
                    MTLColorWriteMask *masks, u32 mask_count,
                    MTLPixelFormat depth_pixel_format,
                    b32 *blending_enabled = 0,
                    b32 can_be_encoded_in_icb = 0)
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
    pipeline_descriptor.supportIndirectCommandBuffers = can_be_encoded_in_icb;

    for(u32 color_attachment_index = 0;
            color_attachment_index < pixel_format_count;
            ++color_attachment_index)
    {
        pipeline_descriptor.colorAttachments[color_attachment_index].pixelFormat = pixel_formats[color_attachment_index];
        pipeline_descriptor.colorAttachments[color_attachment_index].writeMask = masks[color_attachment_index];
        if(blending_enabled)
        {
            pipeline_descriptor.colorAttachments[color_attachment_index].blendingEnabled = 
                blending_enabled[color_attachment_index];
            
            // Not really necessary, but just for the clarification
            if(blending_enabled[color_attachment_index])
            {
                // TODO(gh) Should we support other blending methods?
                // For now, the equation for RGB is : (1-sa)*d + sa*s, and (sa + (1-sa)*da)} for the alpha   
                pipeline_descriptor.colorAttachments[color_attachment_index].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                pipeline_descriptor.colorAttachments[color_attachment_index].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
                pipeline_descriptor.colorAttachments[color_attachment_index].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
                pipeline_descriptor.colorAttachments[color_attachment_index].sourceAlphaBlendFactor = MTLBlendFactorOne;
            }
        }
    }


    pipeline_descriptor.depthAttachmentPixelFormat = depth_pixel_format;

    NSError *error;
    id<MTLRenderPipelineState> result = [device newRenderPipelineStateWithDescriptor:pipeline_descriptor
                                                                error:&error];

    check_ns_error(error);

    [pipeline_descriptor release];

    return result;
}

internal
PLATFORM_WRITE_TO_ENTIRE_TEXTURE(metal_write_to_entire_texture)
{
    MTLRegion region = MTLRegionMake2D(0, 0, width, height);
    [(id<MTLTexture>)texture_handle replaceRegion:region
          mipmapLevel:0
            withBytes:src
          bytesPerRow:width*bytes_per_pixel];
}

// TODO(gh)
// TODO(gh) Should we make this more general, or should we keep this for clarification?
inline MetalTexture2D
metal_make_texture2D(id<MTLDevice> device, MTLPixelFormat pixel_format, i32 width, i32 height, 
                  MTLTextureUsage usage, MTLStorageMode storage_mode)
{
    MTLTextureDescriptor *descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixel_format
                                                                   width:width
                                                                  height:height
                                                               mipmapped:NO];
    descriptor.textureType = MTLTextureType2D;
    descriptor.usage |= usage;
    descriptor.storageMode =  storage_mode;
    descriptor.mipmapLevelCount = 1;

    id<MTLTexture> texture  = [device newTextureWithDescriptor:descriptor];
    [descriptor release];

    MetalTexture2D result = {};
    result.texture = texture;
    result.width = width;
    result.height = height;

    return result;
}

internal
PLATFORM_ALLOCATE_AND_ACQUIRE_TEXTURE_HANDLE(metal_allocate_and_acquire_texture_handle)
{
    // This assumes that every asset has pre-known _unnormalized_ pixel format such as RBGA8 or R8... 
    // 
    MTLPixelFormat pixel_format = MTLPixelFormatInvalid;
    switch(bytes_per_pixel)
    {
        case 1:
        {
            pixel_format = MTLPixelFormatR8Uint;
        }break;

        case 4:
        {
            // TODO(gh) fill this in
        }break;
        default:
        {
            invalid_code_path;
        };
    }
    
    MetalTexture2D texture2D = metal_make_texture2D((id<MTLDevice>)device, pixel_format,width, height, 
                                                    MTLTextureUsageShaderRead, MTLStorageModeShared);

    void *result = (void *)texture2D.texture;
    return result;
}

inline id<MTLDepthStencilState>
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

inline MTLRenderPassDescriptor *
metal_make_renderpass(MTLLoadAction *load_actions, u32 load_action_count,
                      MTLStoreAction *store_actions, u32 store_action_count,
                      id<MTLTexture> *textures, u32 texture_count,
                      v4 *clear_colors, u32 clear_color_count,
                      MTLLoadAction depth_load_action, MTLStoreAction depth_store_action, 
                      id<MTLTexture> depth_texture,
                      f32 depth_clear_value)
{
    assert(load_action_count == store_action_count); 
    if(textures)
    {
        assert(load_action_count == texture_count); 
    }

    if(clear_colors)
    {
        assert(load_action_count == clear_color_count);
    }

    MTLRenderPassDescriptor *result = [MTLRenderPassDescriptor new];

    for(u32 color_attachment_index = 0;
            color_attachment_index < load_action_count;
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

inline id<MTLSamplerState> 
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
       
/////////////// NOTE(gh) compute
inline id<MTLComputePipelineState>
metal_make_compute_pipeline(id<MTLDevice> device, id<MTLLibrary> shader_library, const char *compute_function_name)
{
    id<MTLFunction> compute_function = 
        [shader_library newFunctionWithName:[NSString stringWithUTF8String:compute_function_name]];

    NSError *error = 0;
    id<MTLComputePipelineState> result = [device newComputePipelineStateWithFunction: compute_function 
                                                             error:&error];
    check_ns_error(error);

    return result;
}

inline void
metal_signal_fence_after(id<MTLComputeCommandEncoder> encoder, id<MTLFence> fence)
{
    // Make GPU signal the fence after executing the specified render stage
    [encoder updateFence : fence];
}

inline void
metal_wait_for_fence(id<MTLComputeCommandEncoder> encoder, id<MTLFence> fence)
{
    [encoder waitForFence:fence];
}

inline void
metal_memory_barrier_with_scope(id<MTLComputeCommandEncoder> encoder, MTLBarrierScope scope)
{
    /*
       NOTE(gh) possible barriers 

       MTLBarrierScopeBuffers
       The barrier affects any buffer objects.

       MTLBarrierScopeRenderTargets
       The barrier affects any render targets.

       MTLBarrierScopeTextures
       The barrier affects textures.
    */

    [encoder memoryBarrierWithScope : scope];
}

inline void
metal_set_compute_pipeline(id<MTLComputeCommandEncoder> encoder, id<MTLComputePipelineState> pipeline)
{
    [encoder setComputePipelineState : pipeline];
}

inline void
metal_set_compute_buffer(id<MTLComputeCommandEncoder> encoder, id<MTLBuffer> buffer, u64 offset, u64 index)
{
    [encoder setBuffer : buffer
            offset : offset
            atIndex : index];
}

inline void
metal_set_compute_bytes(id<MTLComputeCommandEncoder> encoder, void *data, u64 length, u64 index)
{
    [encoder setBytes:data 
        length:(NSUInteger)length 
            atIndex:(NSUInteger)index];
}

inline void
metal_dispatch_compute_threads(id<MTLComputeCommandEncoder> encoder, v3u threads_per_grid, v3u threads_per_group)
{
    // NOTE(gh)
    /*
       NOTE(gh)
       threads_per_grid is how many 'threads' should be inside the grid, possibly in x, y, z directions.
       threads_per_group is how many thread should be inside the threadgroup.(maximum value = maxTotalThreadsPerThreadgroup)

        Later, when the GPU actually fires up the threads and starts executing, it will try to group the threads into 
        SIMD group, which is very similar to CPU level SIMD - meaning, all threads inside the simd group will perform
        the same instructions(possibly on different memory). This is what others call the wavefront or warp.
        The size of the lane can be retrieved from threadExecutionWidth, again from the compute pipeline state.

        This is also why it is recommended to not branch inside the kernel, as the thread will do both branchs 
        and mask out as necessary. 

        If the thread group has less threads than the lane size, the GPU will do the same thing as if there were enough threads,
        but just throw away the rest. So in theory, you would want the thread count of the thread group to be multiple of 
        lane size.
    */

    [encoder dispatchThreads : 
        metal_make_mtl_size(threads_per_grid)
        threadsPerThreadgroup : metal_make_mtl_size(threads_per_group)];
}

inline void
metal_end_encoding(id<MTLComputeCommandEncoder> encoder)
{
    [encoder endEncoding];
}

// TODO(gh) Set things properly that were not been documented yet
// i.e meshThreadgroupSizeIsMultipleOfThreadExecutionWidth, payloadMemoryLength
inline id<MTLRenderPipelineState>
metal_make_mesh_render_pipeline(id<MTLDevice> device,
                    const char *pipeline_name, 
                    const char *object_function_name, 
                    const char *mesh_function_name, 
                    const char *fragment_function_name,
                    id<MTLLibrary> shader_library,
                    MTLPrimitiveTopologyClass topology, 
                    MTLPixelFormat *pixel_formats, u32 pixel_format_count, 
                    MTLColorWriteMask *masks, u32 mask_count,
                    MTLPixelFormat depth_pixel_format, 
                    u32 max_object_thread_count_per_object_threadgroup,
                    u32 max_mesh_threadgroup_count_per_mesh_grid,
                    u32 max_mesh_thread_count_per_mesh_threadgroup)
{
    MTLMeshRenderPipelineDescriptor *descriptor = [MTLMeshRenderPipelineDescriptor new];

    descriptor.label = [NSString stringWithUTF8String : pipeline_name];
    descriptor.objectFunction = [shader_library newFunctionWithName : [NSString stringWithUTF8String : object_function_name]];
    descriptor.meshFunction = [shader_library newFunctionWithName : [NSString stringWithUTF8String : mesh_function_name]];
    descriptor.fragmentFunction = [shader_library newFunctionWithName : [NSString stringWithUTF8String : fragment_function_name]];
    descriptor.rasterSampleCount = 1;
    descriptor.rasterizationEnabled = true;
    // descriptor.payloadMemoryLength = ;
    // descriptor.meshThreadgroupSizeIsMultipleOfThreadExecutionWidth = ;

    descriptor.maxTotalThreadsPerObjectThreadgroup = max_object_thread_count_per_object_threadgroup;
    descriptor.maxTotalThreadgroupsPerMeshGrid = max_mesh_threadgroup_count_per_mesh_grid;
    descriptor.maxTotalThreadsPerMeshThreadgroup = max_mesh_thread_count_per_mesh_threadgroup;
   
    for(u32 color_attachment_index = 0;
            color_attachment_index < pixel_format_count;
            ++color_attachment_index)
    {
        descriptor.colorAttachments[color_attachment_index].pixelFormat = pixel_formats[color_attachment_index];
        descriptor.colorAttachments[color_attachment_index].writeMask = masks[color_attachment_index];
    }

    NSError *error;
    // TODO(gh) This is also not documented.. check options and reflection parameter 
    id<MTLRenderPipelineState> result = [device newRenderPipelineStateWithMeshDescriptor: descriptor
                                                                        options : 0
                                                                        reflection : 0
                                                                        error : &error];

    check_ns_error(error);

    [descriptor release];

    return result;
}

inline void
metal_set_object_buffer(id<MTLRenderCommandEncoder> encoder, id<MTLBuffer> buffer, u64 offset, u64 index)
{
    [encoder setObjectBuffer : buffer 
                offset : offset 
                atIndex : index];
}

inline void
metal_set_object_bytes(id<MTLRenderCommandEncoder> render_encoder, void *data, u64 size, u64 index)
{
    [render_encoder setObjectBytes: data 
                    length: size 
                    atIndex: index]; 
}

inline void
metal_set_mesh_buffer(id<MTLRenderCommandEncoder> encoder, id<MTLBuffer> buffer, u64 offset, u64 index)
{
    [encoder setMeshBuffer : buffer 
                offset : offset 
                atIndex : index];
}

inline void
metal_set_mesh_bytes(id<MTLRenderCommandEncoder> encoder, void *data, u64 size, u64 index)
{
    [encoder setMeshBytes: 
                    data 
                    length: size 
                    atIndex: index]; 
}

inline void
metal_draw_mesh_thread_groups(id<MTLRenderCommandEncoder> encoder, v3u object_threadgroups_per_grid, v3u thread_per_object_threadgroup, v3u thread_per_mesh_threadgroup)
{
    [encoder drawMeshThreadgroups : 
            metal_make_mtl_size(object_threadgroups_per_grid)
            threadsPerObjectThreadgroup : metal_make_mtl_size(thread_per_object_threadgroup)
            threadsPerMeshThreadgroup : metal_make_mtl_size(thread_per_mesh_threadgroup)];
}

inline id<MTLFence>
metal_make_fence(id<MTLDevice> device)
{
    id<MTLFence> result = [device newFence];
    return result;
}

inline id<MTLArgumentEncoder>
metal_make_argument_encoder(id<MTLLibrary> shader_library, const char *function_name, u32 index_of_the_argument_in_shader)
{
    id<MTLFunction> function = [shader_library newFunctionWithName:[NSString stringWithUTF8String:function_name]];
    id<MTLArgumentEncoder> result =
        [function newArgumentEncoderWithBufferIndex:index_of_the_argument_in_shader];

    return result;
}

// TODO(joon) only resets 1 command!!!!
inline void
metal_optimize_icb(id<MTLCommandBuffer> command_buffer, id<MTLIndirectCommandBuffer> icb)
{
    id<MTLBlitCommandEncoder> optimize_icb_encoder = [command_buffer blitCommandEncoder];
    optimize_icb_encoder.label = @"Optimize ICB";
    [optimize_icb_encoder optimizeIndirectCommandBuffer:icb
        withRange:NSMakeRange(0, 1)];
    [optimize_icb_encoder endEncoding];
}

inline void
metal_signal_fence_after(id<MTLBlitCommandEncoder> encoder, id<MTLFence> fence)
{
    [encoder updateFence:fence];
}

inline void
metal_wait_for_fence(id<MTLBlitCommandEncoder> encoder, id<MTLFence> fence )
{
    [encoder waitForFence:fence];
}








