#version 450

layout(location = 0) out vec3 nearPoint;
layout(location = 1) out vec3 farPoint;

layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    vec3 pos;
} camera;

vec3 gridPoints[6] = vec3[](
    vec3(-1, -1, 0), vec3(1, -1, 0), vec3(1, 1, 0),
    vec3(-1, -1, 0), vec3(1, 1, 0), vec3(-1, 1, 0)
);

vec3 UnprojectPoint(float x, float y, float z, mat4 viewInv, mat4 projInv) {
    vec4 unprojectedPoint = viewInv * projInv * vec4(x, y, z, 1.0);
    return unprojectedPoint.xyz / unprojectedPoint.w;
}

void main() {
    vec3 p = gridPoints[gl_VertexIndex];
    mat4 viewInv = inverse(camera.view);
    mat4 projInv = inverse(camera.proj);
    nearPoint = UnprojectPoint(p.x, p.y, 0.0, viewInv, projInv);
    farPoint = UnprojectPoint(p.x, p.y, 1.0, viewInv, projInv);
    gl_Position = vec4(p, 1.0);
}
