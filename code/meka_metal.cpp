#include "meka_metal.h"

internal MTLViewport
metal_make_viewport(f32 x, f32 y, f32 width, f32 height, f32 near, f32 far)
{
    MTLViewport result = {};

    result.originX = (f32)x;
    result.originY = (f32)y;
    result.width = (f32)width;
    result.height = (f32)height;
    result.znear = (f32)near;
    result.zfar = (f32)far;

    return result;
}

internal MTLScissorRect
metal_make_scissor_rect(u32 x, u32 y, u32 width, u32 height)
{
    MTLScissorRect result = {};

    result.x = x;
    result.y = y;
    result.width = width;
    result.height = height;

    return result;
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



