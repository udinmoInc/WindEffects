#define WE_CAMERA_BUFFER_SPACE space0
#include "../Common/CameraBuffer.hlsli"
#include "../Common/Color.hlsli"

// Built-in M_DefaultLandscape shading (matte charcoal). Project materials override albedo/params.
cbuffer TerrainMaterial : register(b0, space1)
{
    float4 albedoColor;   // linear RGB + opacity
    float4 lightDirPad;   // xyz = sun direction (world), w = roughness
    float4 materialPad;   // x = metallic, yzw unused
};

struct VSInput
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position    : SV_Position;
    float3 worldPos    : TEXCOORD0;
    float3 worldNormal : TEXCOORD1;
    float2 texCoord    : TEXCOORD2;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    // CPU mesh positions are already in fixed world space (never camera-relative).
    o.worldPos = input.position;
    o.worldNormal = normalize(input.normal);
    o.texCoord = input.texCoord;
    o.position = mul(proj, mul(view, float4(input.position, 1.0)));
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    const float3 albedo = WE_sRGBToLinear(saturate(albedoColor.rgb));
    const float3 normal = normalize(input.worldNormal);
    const float3 lightDir = normalize(lightDirPad.xyz);
    const float roughness = saturate(max(lightDirPad.w, 0.04));
    const float metallic = saturate(materialPad.x);

    // Lightweight matte lighting — high roughness, non-metallic, no debug colors.
    const float3 ambient = albedo * 0.18;
    const float ndotl = saturate(dot(normal, lightDir));
    const float3 diffuse = albedo * ndotl * (1.0 - metallic) * 0.82;

    const float3 viewDir = normalize(cameraPos - input.worldPos);
    const float3 halfV = normalize(lightDir + viewDir);
    const float specPower = lerp(8.0, 128.0, 1.0 - roughness);
    const float spec = pow(saturate(dot(normal, halfV)), specPower) * (0.04 * (1.0 - metallic));

    const float3 lit = ambient + diffuse + spec * albedo;
    return float4(WE_LinearToSRGB(lit), albedoColor.a);
}
