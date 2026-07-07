#ifndef WE_FOUNDATION_PROCEDURAL_SKY_HLSL
#define WE_FOUNDATION_PROCEDURAL_SKY_HLSL

cbuffer CameraBuffer : register(b0, space0)
{
    float4x4 invViewProj;
    float3   cameraPos;
    float    cameraPadding;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 viewDir  : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
  float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
  float4 clip = float4(uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);

  float4 world = mul(invViewProj, clip);
  float3 viewDir = normalize(world.xyz / world.w - cameraPos);

  VSOutput output;
  output.position = float4(clip.xy, 1.0, 1.0);
  output.viewDir = viewDir;
  return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
  float3 dir = normalize(input.viewDir);
  float height = saturate(dir.y * 0.5 + 0.5);

  float3 horizon = float3(0.82, 0.88, 0.94);
  float3 zenith  = float3(0.18, 0.42, 0.82);
  float3 sky = lerp(horizon, zenith, pow(height, 1.35));

  float sunDot = saturate(dot(dir, normalize(float3(0.35, -0.85, 0.25))));
  sky += float3(1.0, 0.92, 0.75) * pow(sunDot, 256.0) * 0.65;

  return float4(sky, 1.0);
}

#endif
