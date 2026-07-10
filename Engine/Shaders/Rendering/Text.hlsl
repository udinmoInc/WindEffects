/**
 * @file Text.hlsl
 * @brief Modern MSDF Text Rendering Shader
 * 
 * Features:
 * - Median distance field evaluation for crisp edges
 * - Derivative-based anti-aliasing using fwidth()
 * - Linear texture sampling (never sRGB)
 * - Pixel range calculation for scale-independent rendering
 * - Gamma correction for accurate color reproduction
 * - Support for RGBA8 UNORM atlas textures
 * - Subpixel rendering for improved legibility
 * - Outline and shadow effects
 */

// ============================================================================
// Vertex Shader
// ============================================================================

struct TextVertexInput {
    [[vk::location(0)]] float2 position : POSITION;     // Screen position
    [[vk::location(1)]] float2 uv : TEXCOORD0;          // Atlas UV coordinates
    [[vk::location(2)]] float4 color : COLOR0;          // Vertex color (RGBA)
    [[vk::location(3)]] float2 texCoord : TEXCOORD1;    // Texture coordinates for effects
    [[vk::location(4)]] float4 params : TEXCOORD2;     // Shader parameters
};

struct TextVertexOutput {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float2 texCoord : TEXCOORD1;
    [[vk::location(3)]] float4 params : TEXCOORD2;
    [[vk::location(4)]] float2 screenPos : TEXCOORD3;  // Screen position for effects
};

struct TextPushConstants {
    float2 projectionScale;      // Projection scale (for DPI scaling)
    float2 translate;            // Translation offset
    float pixelRange;            // MSDF pixel range
    float reserved0;
};

[[vk::push_constant]]
TextPushConstants pc;

TextVertexOutput VSMain(TextVertexInput input) {
    TextVertexOutput output;
    
    // Apply projection and translation
    output.position = float4(input.position * pc.projectionScale + pc.translate, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    output.texCoord = input.texCoord;
    output.params = input.params;
    output.screenPos = input.position;
    
    return output;
}

// ============================================================================
// Fragment Shader
// ============================================================================

[[vk::binding(0, 0)]]
Texture2D<float4> atlasTexture : register(t0);

[[vk::binding(0, 0)]]
SamplerState atlasSampler : register(s0);

/**
 * @brief Median of three values
 * Used to extract the signed distance from MSDF RGB channels
 */
float Median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

/**
 * @brief Calculate screen-space pixel range for MSDF
 * This ensures consistent edge thickness regardless of scale
 */
float ScreenPixelRange(float2 uv, float pixelRange, float2 atlasSize) {
    float2 unitRange = float2(pixelRange, pixelRange) / atlasSize;
    float2 screenTexSize = 1.0 / fwidth(uv);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

/**
 * @brief Sample MSDF and compute signed distance
 */
float SampleMSDF(float2 uv, float pixelRange, float2 atlasSize) {
    float4 msdf = atlasTexture.Sample(atlasSampler, uv);
    float sd = Median(msdf.r, msdf.g, msdf.b);
    float range = ScreenPixelRange(uv, pixelRange, atlasSize);
    return (sd - 0.5) * range;
}

/**
 * @brief Apply anti-aliasing to signed distance
 * Uses smoothstep for smooth edge transitions
 */
float ApplyAA(float distance, float edgeWidth) {
    return saturate((distance + edgeWidth * 0.5) / edgeWidth);
}

/**
 * @brief Apply gamma correction for accurate color reproduction
 */
float3 GammaCorrect(float3 color, float gamma) {
    return pow(color, 1.0 / gamma);
}

/**
 * @brief Apply outline effect
 */
float ApplyOutline(float distance, float outlineWidth, float outlineSoftness) {
    float edge = distance + outlineWidth;
    return smoothstep(edge - outlineSoftness, edge + outlineSoftness, 0.0);
}

/**
 * @brief Apply shadow effect
 */
float ApplyShadow(float2 screenPos, float2 shadowOffset, float shadowSoftness, float shadowOpacity) {
    // This would require a second render pass or screen-space technique
    // For now, return 1.0 (no shadow)
    return 1.0;
}

/**
 * @brief Main fragment shader
 */
float4 PSMain(TextVertexOutput input) : SV_Target {
    // Get atlas dimensions
    uint atlasWidth, atlasHeight;
    atlasTexture.GetDimensions(atlasWidth, atlasHeight);
    float2 atlasSize = float2(atlasWidth, atlasHeight);
    
    // Sample MSDF and compute signed distance
    float distance = SampleMSDF(input.uv, pc.pixelRange, atlasSize);
    
    // Extract shader parameters
    // params.x: edge softness
    // params.y: outline width
    // params.z: outline softness
    // params.w: effect flags (bit 0: outline, bit 1: shadow)
    float edgeSoftness = max(input.params.x, 0.001);
    float outlineWidth = input.params.y;
    float outlineSoftness = max(input.params.z, 0.001);
    uint effectFlags = asuint(input.params.w);
    
    // Apply anti-aliasing
    float opacity = ApplyAA(distance, edgeSoftness);
    
    // Apply outline if enabled
    if ((effectFlags & 0x01) != 0 && outlineWidth > 0.0) {
        float outlineAlpha = ApplyOutline(distance, outlineWidth, outlineSoftness);
        opacity = max(opacity, outlineAlpha * 0.5);
    }
    
    // Apply subpixel rendering for improved legibility
    // This is a simplified subpixel anti-aliasing
    float2 offset = fwidth(input.uv);
    float subpixelAA = 1.0;
    if (offset.x > 0.0) {
        float leftDist = SampleMSDF(input.uv + float2(-offset.x, 0.0), pc.pixelRange, atlasSize);
        float rightDist = SampleMSDF(input.uv + float2(offset.x, 0.0), pc.pixelRange, atlasSize);
        subpixelAA = smoothstep(-edgeSoftness, edgeSoftness, 
                               (leftDist + distance + rightDist) * 0.333);
        opacity = min(opacity, subpixelAA);
    }
    
    // Apply gamma correction (1.1 is a good default for MSDF)
    opacity = pow(opacity, 1.0 / 1.1);
    
    // Premultiply alpha for correct blending
    float3 rgb = input.color.rgb * input.color.a;
    float alpha = input.color.a * opacity;
    
    return float4(rgb, alpha);
}

// ============================================================================
// Alternative Fragment Shader for SDF (single-channel)
// ============================================================================

float SampleSDF(float2 uv, float pixelRange, float2 atlasSize) {
    float sdf = atlasTexture.Sample(atlasSampler, uv).r;
    float range = ScreenPixelRange(uv, pixelRange, atlasSize);
    return (sdf - 0.5) * range;
}

float4 PSMain_SDF(TextVertexOutput input) : SV_Target {
    uint atlasWidth, atlasHeight;
    atlasTexture.GetDimensions(atlasWidth, atlasHeight);
    float2 atlasSize = float2(atlasWidth, atlasHeight);
    
    float distance = SampleSDF(input.uv, pc.pixelRange, atlasSize);
    float edgeSoftness = max(input.params.x, 0.001);
    float opacity = ApplyAA(distance, edgeSoftness);
    opacity = pow(opacity, 1.0 / 1.1);
    
    float3 rgb = input.color.rgb * input.color.a;
    float alpha = input.color.a * opacity;
    
    return float4(rgb, alpha);
}

// ============================================================================
// Debug Visualization Shader
// ============================================================================

float4 PSMain_Debug(TextVertexOutput input) : SV_Target {
    float4 msdf = atlasTexture.Sample(atlasSampler, input.uv);
    
    // Visualize RGB channels
    return msdf;
}

float4 PSMain_Debug_Distance(TextVertexOutput input) : SV_Target {
    uint atlasWidth, atlasHeight;
    atlasTexture.GetDimensions(atlasWidth, atlasHeight);
    float2 atlasSize = float2(atlasWidth, atlasHeight);
    
    float distance = SampleMSDF(input.uv, pc.pixelRange, atlasSize);
    
    // Visualize signed distance (red = outside, green = edge, blue = inside)
    float3 debugColor;
    if (distance < -0.5) {
        debugColor = float3(1.0, 0.0, 0.0); // Outside
    } else if (distance > 0.5) {
        debugColor = float3(0.0, 0.0, 1.0); // Inside
    } else {
        debugColor = float3(0.0, 1.0, 0.0); // Edge
    }
    
    return float4(debugColor, 1.0);
}
