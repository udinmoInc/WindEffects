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

// Screen-space anti-aliasing width for analytic SDF shapes (crisp subpixel feathering band).
float sdfFeather(float dist)
{
    float fw = max(fwidth(dist), 0.0001);
    return fw * 0.75;
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

float4 PSMain(VSOutput input) : SV_Target
{
    uint iType = (uint)(input.sdfParams.y + 0.5);
    
    // Type 5.0: Solid quad (lines, untextured rects).
    if (iType == 5u)
    {
        return input.color;
    }

    // Type 0.0 & Type 4.0: Textures & Icons.
    if (iType == 0u || iType == 4u)
    {
        float4 texColor = texSampler.Sample(samp0, input.uv);
        return float4(texColor.rgb * input.color.rgb, texColor.a * input.color.a);
    }

    // Type 3.0: MSDF text.
    if (iType == 3u)
    {
        float4 texColor = texSampler.Sample(samp0, input.uv);
        float sd = median3(texColor.r, texColor.g, texColor.b);
        uint atlasW = 0;
        uint atlasH = 0;
        texSampler.GetDimensions(atlasW, atlasH);
        float spr = screenPxRange(input.uv, max(input.sdfParams.z, 1.0), float2(atlasW, atlasH));
        float opacity = saturate((sd - 0.5) * spr + 0.5);
        if (opacity <= 0.001)
        {
            discard;
        }
        return float4(input.color.rgb, input.color.a * opacity);
    }

    // Type 1.0 (Rect Fill) & Type 2.0 (Inset Border Ring).
    float2 center = float2(input.sdfRect.x + input.sdfRect.z * 0.5, input.sdfRect.y + input.sdfRect.w * 0.5);
    float2 halfSize = float2(input.sdfRect.z * 0.5, input.sdfRect.w * 0.5);
    float radius = input.sdfParams.x;
    float2 p = input.worldPos - center;

    float alpha;
    if (iType == 2u)
    {
        // Type 2.0: Exact inset rounded border ring math.
        float thickness = max(input.sdfParams.z, 1.0);
        float2 outerHalf = halfSize;
        float outerR = radius;
        float2 innerHalf = max(halfSize - thickness, 0.0);
        float innerR = max(radius - thickness, 0.0);

        float2 qOuter = abs(p) - outerHalf + outerR;
        float dOuter = min(max(qOuter.x, qOuter.y), 0.0) + length(max(qOuter, 0.0)) - outerR;

        float2 qInner = abs(p) - innerHalf + innerR;
        float dInner = min(max(qInner.x, qInner.y), 0.0) + length(max(qInner, 0.0)) - innerR;

        float ringDist = max(dOuter, -dInner);
        alpha = sdfFillAlpha(ringDist);
    }
    else
    {
        float2 q = abs(p) - halfSize + radius;
        float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;
        alpha = sdfFillAlpha(dist);
    }

    if (alpha <= 0.001)
    {
        discard;
    }

    float4 outColor = input.color;
    outColor.a *= alpha;
    return outColor;
}
