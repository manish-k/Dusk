#version 450

#extension GL_KHR_vulkan_glsl : enable
#extension GL_EXT_multiview : enable

layout(location = 0) out vec3 fragUVW;

layout (set = 1, binding = 0) buffer ProjView 
{
	mat4 projViewMatrix;
} projView[6];

// Cube vertices
const vec3 positions[8] = vec3[8](
    vec3(-1.0, -1.0, -1.0),
    vec3( 1.0, -1.0, -1.0),
    vec3( 1.0,  1.0, -1.0),
    vec3(-1.0,  1.0, -1.0),
    vec3(-1.0, -1.0,  1.0),
    vec3( 1.0, -1.0,  1.0),
    vec3( 1.0,  1.0,  1.0),
    vec3(-1.0,  1.0,  1.0)
);

// Cube indices
const uint indices[36] = uint[36](
    0, 1, 2, 2, 3, 0, // front
    1, 5, 6, 6, 2, 1, // right
    5, 4, 7, 7, 6, 5, // back
    4, 0, 3, 3, 7, 4, // left
    3, 2, 6, 6, 7, 3, // top
    4, 5, 1, 1, 0, 4  // bottom
);

void main() {
    uint idx = indices[gl_VertexIndex];
    vec3 pos = positions[idx];
    
    fragUVW = pos;
    
    gl_Position = projView[gl_ViewIndex].projViewMatrix * vec4(pos, 1.0);
}