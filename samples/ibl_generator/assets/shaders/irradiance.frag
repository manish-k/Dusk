#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec3 fragUVW;

layout(location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform samplerCube textures[];

const float PI = 3.14159265359;

layout(push_constant) uniform IBLPushConstant {
    uint envCubeMapTexId;
    uint sampleCount;
    float roughness;
    uint resolution;
} push;

void main()
{
    uint envCubeMapTexIdx = nonuniformEXT(push.envCubeMapTexId);

    // normal direction representing hemisphere orientation
    vec3 normal = normalize(fragUVW);
    vec3 irradiance = vec3(0.0); 

    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, normal));
    up         = normalize(cross(normal, right));

    float sampleDelta = 0.025;
    float nrSamples = 0.0; 
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            // tangent space to world
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal; 

            irradiance += texture(textures[envCubeMapTexIdx], sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));

    outColor = vec4(irradiance.rgb, 1.0);
}