#version 450

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec4 inSdfRect;
layout(location = 4) in vec4 inSdfParams;

layout(push_constant) uniform PushConstants {
    vec2 uScale;
    vec2 uTranslate;
} pc;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec4 outSdfRect;
layout(location = 3) out vec4 outSdfParams;
layout(location = 4) out vec2 outWorldPos;

void main() {
    outUV = inUV;
    outColor = inColor;
    outSdfRect = inSdfRect;
    outSdfParams = inSdfParams;
    outWorldPos = inPos; // Pass the raw screen-space position
    gl_Position = vec4(inPos * pc.uScale + pc.uTranslate, 0.0, 1.0);
}
