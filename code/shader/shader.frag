#version 450

layout(location = 0) in vec3 fragP;
layout(location = 1) in vec4 viewP;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(fragP, 1.0f);
}
