#version 450

layout(location = 0) out vec2 outUV;

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0), vec2( 1.0, -1.0), vec2( 1.0,  1.0),
    vec2(-1.0, -1.0), vec2( 1.0,  1.0), vec2(-1.0,  1.0)
);

void main() {
    vec2 p = positions[gl_VertexIndex];
    outUV = p * 0.5 + 0.5; // Translate NDC to [0, 1] UV
    gl_Position = vec4(p, 1.0, 1.0); // Render at the far plane depth (1.0)
}
