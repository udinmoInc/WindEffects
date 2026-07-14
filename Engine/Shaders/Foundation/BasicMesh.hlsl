#ifndef WE_FOUNDATION_BASIC_MESH_HLSL
#define WE_FOUNDATION_BASIC_MESH_HLSL

#include "../Common/Color.hlsli"
#include "../Common/CameraBuffer.hlsli"

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
};

cbuffer ObjectBuffer : register(b0, space1)
{
    float4x4 model;
    float4   color;
};

cbuffer LightBuffer : register(b0, space2)
{
    float3 lightDirection;
    float  lightIntensity;
    float3 lightColor;
    float  lightPadding;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(model, float4(input.position, 1.0));
    output.worldPos = worldPos.xyz;
    output.worldNormal = normalize(mul((float3x3)model, input.normal));
    output.position = mul(proj, mul(view, worldPos));
    return output;
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 albedo = saturate(color.rgb);
    float3 normal = normalize(input.worldNormal);
    // lightDirection is travel direction; reverse for N·L.
    float3 lightDir = normalize(-lightDirection);
    float3 viewDir = normalize(cameraPos - input.worldPos);

    float ndotl = saturate(dot(normal, lightDir));
    float3 diffuse = albedo * lightColor * lightIntensity * ndotl;

    float3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(saturate(dot(normal, halfDir)), 64.0);
    float3 specular = lightColor * spec * 0.04 * lightIntensity;

    float3 ambient = albedo * float3(0.04, 0.05, 0.06);
    float3 lit = ambient + diffuse + specular;

    // Display-referred encode (foundation path has no post exposure/tonemap).
    float3 compressed = lit / (1.0 + lit * 0.35);
    return float4(WE_LinearToSRGB(compressed), color.a);
}

#endif
