#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexCoord;

layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    vec3 pos;
} camera;

layout(binding = 1) uniform ObjectBuffer {
    mat4 model;
    vec4 color;
    int mode; // 0 = Lit, 1 = Unlit, 2 = Wireframe
} object;

void main() {
    vec4 worldPos = object.model * vec4(inPosition, 1.0);
    outWorldPos = worldPos.xyz;
    outNormal = normalize(mat3(transpose(inverse(object.model))) * inNormal);
    outTexCoord = inTexCoord;
    gl_Position = camera.proj * camera.view * worldPos;
}
