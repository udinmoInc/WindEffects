#version 450

layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSdfRect;
layout(location = 3) in vec4 inSdfParams;
layout(location = 4) in vec2 inWorldPos;

layout(binding = 0) uniform sampler2D texSampler;
layout(location = 0) out vec4 outColor;

// SDF for a rounded box
float udRoundBox(vec2 p, vec2 b, float r) {
    return length(max(abs(p) - b + r, 0.0)) - r;
}

void main() {
    float type = inSdfParams.y;
    if (type < 0.5) {
        // Standard textured or plain quad
        outColor = inColor * texture(texSampler, inUV);
    } else {
        // SDF Rounded Rectangle
        vec2 center = vec2(inSdfRect.x + inSdfRect.z * 0.5, inSdfRect.y + inSdfRect.w * 0.5);
        vec2 halfSize = vec2(inSdfRect.z * 0.5, inSdfRect.w * 0.5);
        float radius = inSdfParams.x;
        
        float dist = udRoundBox(inWorldPos - center, halfSize, radius);
        
        // Anti-aliasing soft edge
        float softness = 1.0; 
        float alpha = 1.0 - smoothstep(-softness, softness, dist);
        
        // Output with border/shadows logic can go here in the future
        outColor = inColor;
        outColor.a *= alpha;
    }
}
