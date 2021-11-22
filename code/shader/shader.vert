#version 450

struct vertex
{
    // storage buffer has vec4 alignment for vec3,
    // so if we want to use vec3, this is the way...
    float pX;
    float pY;
    float pZ;

    float nX;
    float nY;
    float nZ;

    //float fX;
    //float fY;
    //float fZ;
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
    vec3 lightP;
}uniformBuffer;

layout(location = 0) out vec3 fragWorldP;
layout(location = 1) out vec3 fragWorldNormal;

void main() 
{
    vertex vertex = vertices[gl_VertexIndex];
    vec3 p = vec3(vertex.pX, vertex.pY, vertex.pZ);

    gl_Position = uniformBuffer.projView*uniformBuffer.model*vec4(p, 1.0f);;

    fragWorldP = vec3(uniformBuffer.model*vec4(p, 1.0f));
    fragWorldNormal = normalize(vec3(uniformBuffer.model*vec4(vertex.nX, vertex.nY, vertex.nZ, 0.0f)));
}
