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

[[vk::push_constant]]
UIPushConstants pc;

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

// Smoothstep for anti-aliasing
float smoothstepAA(float edge0, float edge1, float x)
{
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
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

        // Basic gamma-aware edge response to reduce dark fringes/halos.
        opacity = pow(opacity, 1.0 / 1.1);

        float4 outColor = input.color;
        outColor.rgb *= outColor.a;
        outColor.a *= opacity;
        return outColor;
    }

    // Type 0.0 is Texture/Icon bitmap.
    // Icons/thumbnails store coverage in alpha; vertex color carries the tint.
    if (type < 0.5)
    {
        uint atlasW = 0, atlasH = 0;
        texSampler.GetDimensions(atlasW, atlasH);
        
        // 1 px offset in Y for depth effect
        float2 dy = float2(0.0, 1.0 / (float)atlasH);

        float coverage = texSampler.Sample(samp0, input.uv).a;
        float shadowCov = texSampler.Sample(samp0, input.uv - dy).a;
        float highCov = texSampler.Sample(samp0, input.uv + dy).a;

        // Shadow is black, opacity ~10%
        float4 shadowLayer = float4(0.0, 0.0, 0.0, shadowCov * 0.10);
        // Highlight is white, opacity ~12%
        float4 highlightLayer = float4(1.0, 1.0, 1.0, highCov * 0.12);
        
        // Base icon layer
        float4 iconLayer = float4(input.color.rgb, input.color.a * coverage);

        // Composite bottom-up: Shadow -> Highlight -> Icon
        // 1. Shadow + Highlight
        float outAlpha1 = highlightLayer.a + shadowLayer.a * (1.0 - highlightLayer.a);
        float3 outColor1 = (highlightLayer.rgb * highlightLayer.a + shadowLayer.rgb * shadowLayer.a * (1.0 - highlightLayer.a)) / max(outAlpha1, 0.00001);
        
        // 2. (Shadow + Highlight) + Icon
        float finalAlpha = iconLayer.a + outAlpha1 * (1.0 - iconLayer.a);
        float3 finalColor = (iconLayer.rgb * iconLayer.a + outColor1 * outAlpha1 * (1.0 - iconLayer.a)) / max(finalAlpha, 0.00001);
        
        return float4(finalColor, finalAlpha);
    }

    // Type 4.0 is a full-color texture (viewport / render targets).
    if (type > 3.5 && type < 4.5)
    {
        float4 texColor = texSampler.Sample(samp0, input.uv);
        return texColor * input.color;
    }

    // Type 1.0 is Rect, Type 2.0 is Border
    float2 center = float2(input.sdfRect.x + input.sdfRect.z * 0.5, input.sdfRect.y + input.sdfRect.w * 0.5);
    float2 halfSize = float2(input.sdfRect.z * 0.5, input.sdfRect.w * 0.5);
    float radius = input.sdfParams.x;
    
    float2 p = input.worldPos - center;
    float2 q = abs(p) - halfSize + radius;
    float dist = min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - radius;

    float alpha;
    if (type > 1.5)
    {
        float thickness = max(input.sdfParams.z, 1.0);
        float edgeDist = abs(dist) - thickness * 0.5;
        alpha = 1.0 - smoothstepAA(0.0, 1.0, edgeDist);
    }
    else
    {
        alpha = 1.0 - smoothstepAA(-1.0, 1.0, dist);
    }

    float4 outColor = input.color;
    outColor.a *= alpha;
    
    return outColor;
}
