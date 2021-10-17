#version 450

layout(location = 0) in vec3 fragP;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(0.2f, 1.0f, 1.0f, 1.0f);
}
