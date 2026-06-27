#version 450

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() {
    // Elegant viewport dark background gradient
    vec3 topColor = vec3(0.08, 0.08, 0.1);
    vec3 bottomColor = vec3(0.18, 0.18, 0.2);
    vec3 color = mix(bottomColor, topColor, inUV.y);
    outColor = vec4(color, 1.0);
}
