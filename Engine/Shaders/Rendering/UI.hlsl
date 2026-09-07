// Vertex COLOR is linear RGB (converted from sRGB authoring in OverlayRenderer).
// The swapchain uses an sRGB format; the GPU encodes linear shader output to sRGB storage.
struct VSInput
{
    [[vk::location(0)]] float2 position : POSITION0;
    [[vk::location(1)]] float2 uv       : TEXCOORD0;
    [[vk::location(2)]] float4 color    : COLOR0;
    [[vk::location(3)]] float4 sdfRect  : TEXCOORD1;
    [[vk::location(4)]] float4 sdfParams: TEXCOORD2;
};

struct VSOutput
{
    float4 position                     : SV_Position;
    [[vk::location(0)]] float2 uv       : TEXCOORD0;
    [[vk::location(1)]] float4 color    : COLOR0;
    [[vk::location(2)]] float4 sdfRect  : TEXCOORD1;
    [[vk::location(3)]] float4 sdfParams: TEXCOORD2;
    [[vk::location(4)]] float2 worldPos : TEXCOORD3;
};

struct UIPushConstants
{
    float2 uScale;
    float2 uTranslate;
};

#if defined(WE_TARGET_DXIL)
cbuffer UIPushConstantBuffer : register(b0, space0)
{
    UIPushConstants pc;
};
#else
[[vk::push_constant]]
UIPushConstants pc;
#endif

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.uv = input.uv;
    o.color = input.color;
    o.sdfRect = input.sdfRect;
    o.sdfParams = input.sdfParams;
    o.worldPos = input.position;
    o.position = float4(input.position * pc.uScale + pc.uTranslate, 0.0, 1.0);
    return o;
}

// The UI pipeline uses a single combined image sampler at set=0, binding=0.
[[vk::binding(0, 0)]]
Texture2D    texSampler : register(t0, space0);
SamplerState samp0      : register(s0, space0);

// Signed distance field for rounded rectangle
float sdRoundBox(float2 p, float2 b, float r)
{
    float2 q = abs(p) - b + r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r;
}

// Screen-space anti-aliasing width for analytic SDF shapes (~1 pixel feather).
float sdfFeather(float dist)
{
    return max(fwidth(dist), 0.75);
}

// Smooth coverage for filled SDF shapes (rects, shadows).
float sdfFillAlpha(float dist)
{
    float w = sdfFeather(dist);
    return 1.0 - smoothstep(-w * 0.5, w * 0.5, dist);
}

// Smooth coverage for SDF border rings.
float sdfBorderAlpha(float edgeDist)
{
    float w = sdfFeather(edgeDist);
    return 1.0 - smoothstep(-w * 0.5, w * 0.5, edgeDist);
}

float median3(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(float2 uv, float pxRange, float2 atlasSize)
{
    float2 unitRange = float2(pxRange, pxRange) / atlasSize;
    float2 screenTexSize = 1.0 / fwidth(uv);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

// High-frequency pseudo-random hash for pixel-scale micro-texture
float microTextureHash(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

// Fine monochrome charcoal matte texture (2-4% subtle tonal variation for premium rough surface)
float3 applyCharcoalMatteTexture(float3 baseColor, float2 screenPos)
{
    float n1 = microTextureHash(screenPos);
    float n2 = microTextureHash(screenPos + float2(13.1, 7.9));
    float noise = (n1 * 0.6 + n2 * 0.4) - 0.5;
    
    // 2.5% subtle monochrome tonal variation
    float delta = noise * 0.025;
    return saturate(baseColor + delta);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float type = input.sdfParams.y;
    
    // Type 3.0 is MSDF text.
    if (type > 2.5 && type < 3.5)
    {
        float4 texColor = texSampler.Sample(samp0, input.uv);
        float sd = median3(texColor.r, texColor.g, texColor.b);
        uint atlasW = 0;
        uint atlasH = 0;
        texSampler.GetDimensions(atlasW, atlasH);
        float spr = screenPxRange(input.uv, max(input.sdfParams.z, 1.0), float2(atlasW, atlasH));
        float opacity = saturate((sd - 0.5) * spr + 0.5);

        float4 outColor = input.color;
        outColor.a *= opacity;
        return outColor;
    }

    // Type 0.0 is Texture/Icon bitmap.
    // Sample texture RGB * vertex tint color; alpha is modulated by vertex alpha.
    // Preserves 2.5D shading, highlights, and drop shadows while applying theme tint.
    if (type < 0.5)
    {
        float4 texColor = texSampler.Sample(samp0, input.uv);
        return float4(texColor.rgb * input.color.rgb, texColor.a * input.color.a);
    }

    // Type 5.0 is a solid quad (e.g. lines, untextured rects).
    if (type > 4.5 && type < 5.5)
    {
        float3 color = input.color.rgb;
        if (input.color.a > 0.9 && max(color.r, max(color.g, color.b)) < 0.35)
        {
            color = applyCharcoalMatteTexture(color, input.worldPos);
        }
        return float4(color, input.color.a);
    }

    // Type 4.0 is a full-color texture (WindIcons / viewports).
    // Sample authored RGB+A; vertex color modulates tint (white = unchanged).
    if (type > 3.5 && type < 4.5)
    {
        float4 texColor = texSampler.Sample(samp0, input.uv);
        return float4(texColor.rgb * input.color.rgb, texColor.a * input.color.a);
    }

    // Type 1.0 is Rect, Type 2.0 is Border
    // sdfParams.w = 1.0 requests a hard coverage mask (no SDF feather) for opaque theme fills.
    const bool opaqueHard = input.sdfParams.w > 0.5;
    float2 center = float2(input.sdfRect.x + input.sdfRect.z * 0.5, input.sdfRect.y + input.sdfRect.w * 0.5);
    float2 halfSize = float2(input.sdfRect.z * 0.5, input.sdfRect.w * 0.5);
    float radius = input.sdfParams.x;
    
    float2 p = input.worldPos - center;
    float2 q = abs(p) - halfSize + radius;
    float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;

    float alpha;
    if (type > 1.5)
    {
        // Type 2 is a stroke only. Hard-fill coverage would paint the interior of pills/inputs.
        float thickness = max(input.sdfParams.z, 1.0);
        float edgeDist = abs(dist) - thickness * 0.5;
        alpha = sdfBorderAlpha(edgeDist);
    }
    else
    {
        if (opaqueHard) {
            if (dist > 0.0) {
                discard;
            }
            float3 color = input.color.rgb;
            if (max(color.r, max(color.g, color.b)) < 0.35)
            {
                color = applyCharcoalMatteTexture(color, input.worldPos);
            }
            return float4(color, 1.0);
        }
        alpha = sdfFillAlpha(dist);
    }

    float4 outColor = input.color;
    if (outColor.a > 0.9 && max(outColor.r, max(outColor.g, outColor.b)) < 0.35)
    {
        outColor.rgb = applyCharcoalMatteTexture(outColor.rgb, input.worldPos);
    }
    outColor.a *= alpha;
    
    return outColor;
}
