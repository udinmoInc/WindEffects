// UE5 Slate-inspired UI Shaders
// Completely independent from scene rendering pipeline

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

Texture2D    texSampler : register(t0, space0);
SamplerState samp0      : register(s0, space0);

// Signed distance field for rounded rectangle
float udRoundBox(float2 p, float2 b, float r)
{
    return length(max(abs(p) - b + r, 0.0)) - r;
}

// Smoothstep for anti-aliasing
float smoothstepAA(float edge0, float edge1, float x)
{
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float type = input.sdfParams.y;
    
    // Type 0: Textured quad (text, icons, images).
    // Use sampled coverage as alpha mask and tint with command color.
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
    
    // Type 1: SDF rounded rectangle (solid UI chrome)
    // Type 2: SDF rounded outline (borders)
    
    float2 center = float2(input.sdfRect.x + input.sdfRect.z * 0.5, 
                          input.sdfRect.y + input.sdfRect.w * 0.5);
    float2 halfSize = float2(input.sdfRect.z * 0.5, input.sdfRect.w * 0.5);
    float radius = input.sdfParams.x;
    float dist = udRoundBox(input.worldPos - center, halfSize, radius);
    
    float alpha;
    if (type > 1.5)
    {
        // Rounded outline
        float thickness = max(input.sdfParams.z, 1.0);
        float edgeDist = abs(dist) - thickness * 0.5;
        alpha = 1.0 - smoothstepAA(0.0, 1.0, edgeDist);
    }
    else
    {
        // Filled rounded rectangle
        alpha = 1.0 - smoothstepAA(-1.0, 1.0, dist);
    }
    
    float4 outColor = input.color;
    outColor.a *= alpha;
    return outColor;
}
