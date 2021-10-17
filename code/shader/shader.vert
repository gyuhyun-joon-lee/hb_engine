#version 450

layout(location = 0) in float x;
layout(location = 1) in float y;
layout(location = 2) in float z;

layout(location = 0) out vec3 fragP;


void main() 
{
    vec3 p = vec3(x, y, z);
    gl_Position = vec4(p, 1.0);
    fragP = p;
}
