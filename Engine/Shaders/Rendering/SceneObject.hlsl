#include "../Common/Camera.hlsli"
#include "../Common/Color.hlsli"

struct VSInput
{
    float3 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position   : SV_Position;
    float3 worldPos   : TEXCOORD0;
    float3 worldNormal: TEXCOORD1;
    float2 texCoord   : TEXCOORD2;
};

cbuffer CameraBuffer : register(b0, space0)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

cbuffer ObjectBuffer : register(b1, space0)
{
    float4x4 model;
    float4   color;
    int      mode;
    int3     objectPadding;
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    float4 worldPos = mul(model, float4(input.position, 1.0));
    o.worldPos = worldPos.xyz;
    o.worldNormal = normalize(mul(model, float4(input.normal, 0.0)).xyz);
    o.texCoord = input.texCoord;
    o.position = mul(proj, mul(view, worldPos));
    return o;
}

float4 PSMain(VSOutput input) : SV_Target
{
    const float3 albedo = WE_sRGBToLinear(saturate(color.rgb));

    if (mode == 1 || mode == 2)
    {
        const float3 mapped = WE_ApplyFilmicTonemap(albedo, WE_ExposureFromEV100(1.85));
        return float4(WE_LinearToSRGB(mapped), color.a);
    }

    float3 normal = normalize(input.worldNormal);
    float3 lightDir = normalize(float3(0.38, 0.92, 0.18));
    float3 viewDir = normalize(cameraPos - input.worldPos);

    // Dark editor ambience: hemi ambient + soft key + very low spec.
    const float upN = saturate(normal.y * 0.5 + 0.5);
    const float3 hemiGround = float3(0.006, 0.006, 0.006);
    const float3 hemiSky = float3(0.018, 0.018, 0.018);
    const float3 ambient = lerp(hemiGround, hemiSky, upN);

    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * float3(0.11, 0.11, 0.11);

    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    float3 specular = 0.03 * spec * float3(1.0, 1.0, 1.0);

    float3 litLinear = albedo * (ambient + diffuse) + specular;

    // Gentle aerial perspective to reveal depth without brightening the scene.
    const float distToCamera = length(cameraPos - input.worldPos);
    const float haze = 1.0 - exp(-distToCamera * 0.0045);
    const float3 hazeColor = float3(0.010, 0.010, 0.010);
    litLinear = lerp(litLinear, hazeColor, saturate(haze * 0.45));

    const float3 mapped = WE_ApplyFilmicTonemap(litLinear, WE_ExposureFromEV100(1.85));
    return float4(WE_LinearToSRGB(mapped), color.a);
}
