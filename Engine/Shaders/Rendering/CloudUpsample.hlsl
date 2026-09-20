#ifndef WE_CLOUD_UPSAMPLE_HLSL
#define WE_CLOUD_UPSAMPLE_HLSL

// Fullscreen upsample/composite of half-res volumetric radiance (premultiplied RGBA).

Texture2D    cloudHalfRes : register(t0, space0);
SamplerState cloudSamp    : register(s0, space0);

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float2 clipXY = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);

    VSOutput o;
    o.position = float4(clipXY, 0.0, 1.0);
    o.uv = float2(clipXY.x * 0.5 + 0.5, clipXY.y * 0.5 + 0.5);
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    return cloudHalfRes.SampleLevel(cloudSamp, input.uv, 0.0);
}

#endif // WE_CLOUD_UPSAMPLE_HLSL
