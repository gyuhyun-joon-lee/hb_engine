#version 450

struct vertex
{
    float x, y, z;
    //float nx, ny, nz;
    //float tu, tv;
};

// Storage Buffer
layout(binding = 0) buffer Vertices
{
    vertex vertices[];
};

layout(binding = 1) uniform uniform_buffer
{
    mat4 model; 
    mat4 projView;
}uniformBuffer;

layout(location = 0) out vec3 fragP;
layout(location = 1) out vec4 viewP;

void main() 
{
    vertex vertex = vertices[gl_VertexIndex];

    vec4 p = uniformBuffer.projView*uniformBuffer.model*vec4(vertex.x, vertex.y, vertex.z, 1.0f);
    gl_Position = p;

    fragP = p.xyz;
}
