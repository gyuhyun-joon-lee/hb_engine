#version 450

layout(location = 0) in vec3 fragP;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform uniform_buffer
{
    mat4 model; 
    mat4 projView;
    vec3 lightP;
}uniformBuffer;

void main() 
{
    vec3 lightColor = vec3(1, 1, 1);
    vec3 lightDir = normalize(uniformBuffer.lightP - fragP);

    float distanceSquare = dot(uniformBuffer.lightP, fragP);
    //float attenuation = 10000.0f/distanceSquare;

    vec3 ambient = lightColor * 0.0f;
    vec3 diffuse = lightColor*max(dot(lightDir, fragNormal), 0.0f);

    vec3 resultColor = ambient + diffuse;
    outColor = vec4(resultColor, 1.0f);
}
