#version 450

layout(location = 0) in vec2 fragUV;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D uAlbedo;

void main() {
    vec3 albedo = texture(uAlbedo, fragUV).rgb;

    outColor = vec4(albedo, 1.0);
}