#version 450

layout(location = 0) in vec3 nearPoint;
layout(location = 1) in vec3 farPoint;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform CameraBuffer {
    mat4 view;
    mat4 proj;
    vec3 pos;
} camera;

vec4 grid(vec3 fragPos3D, float scale, bool drawAxis) {
    vec2 coord = fragPos3D.xz * scale;
    vec2 derivative = fwidth(coord);
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / derivative;
    float line = min(grid.x, grid.y);
    float minimumz = min(derivative.y, 1.0);
    float minimumx = min(derivative.x, 1.0);
    vec4 color = vec4(0.3, 0.3, 0.3, 1.0 - min(line, 1.0));
    
    if (drawAxis) {
        // X axis is Red (along Z = 0)
        if (fragPos3D.z > -0.05 * minimumz && fragPos3D.z < 0.05 * minimumz) {
            color = vec4(0.8, 0.1, 0.1, 1.0 - min(grid.y, 1.0));
        }
        // Z axis is Blue (along X = 0)
        if (fragPos3D.x > -0.05 * minimumx && fragPos3D.x < 0.05 * minimumx) {
            color = vec4(0.1, 0.1, 0.8, 1.0 - min(grid.x, 1.0));
        }
    }
    return color;
}

void main() {
    if (abs(farPoint.y - nearPoint.y) < 1e-6) discard;
    
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    if (t < 0.0) discard;
    
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);
    
    vec4 clip_space_pos = camera.proj * camera.view * vec4(fragPos3D, 1.0);
    float ndc_depth = clip_space_pos.z / clip_space_pos.w;
    gl_FragDepth = ndc_depth;
    
    // Smoothly fade the grid in the distance
    float dist = length(fragPos3D.xz - camera.pos.xz);
    float fade = max(0.0, 1.0 - (dist / 120.0));
    
    // Draw major lines (1.0 scale) and minor lines (10.0 scale)
    vec4 grid1 = grid(fragPos3D, 1.0, true);
    vec4 grid10 = grid(fragPos3D, 10.0, false);
    
    vec4 gridColor = grid10 * 0.3 + grid1 * 0.7;
    gridColor.a *= fade;
    
    if (gridColor.a < 0.02) discard;
    
    outColor = gridColor;
}
