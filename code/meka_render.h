#ifndef MEKA_RENDER_H
#define MEKA_RENDER_H

// TODO(joon) : Any need to have mulitiple types of vertices,
// i.e if the vertex is very small, does it increae the vertex cache hit chance?
struct vertex
{
    v3 p;
    v3 normal;
    //v3 faceNormal;
};
    // TODO(joon) : Texturecoord?

// NOTE(joon) : This forces us to copy these datas again to the actual 'usable' vertex buffer,
// but even with more than 100000 vertices, the size of it isn't too big 
struct loaded_raw_mesh
{
    v3 *positions;
    u32 positionCount;

    v3 *normals;
    u32 normalCount;

    v2 *textureCoords;
    u32 textureCoordCount;

    u16 *indices;
    u32 indexCount;
};

// mesh that will be consumed by the renderer
struct render_mesh
{
    vk_host_visible_coherent_buffer vertexBuffer;
    vk_host_visible_coherent_buffer indexBuffer;
    u32 indexCount;
};

struct camera
{
    v3 p;
    union
    {
        struct
        {
            r32 alongX;
            r32 alongY;
            r32 alongZ;
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

struct uniform_buffer
{
    // NOTE(joon) : mat4 should be 16 bytes aligned
    alignas(16) m4 model;
    alignas(16) m4 projView;
    alignas(16) v3 lightP;
};

#endif
