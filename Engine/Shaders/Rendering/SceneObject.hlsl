#include "../Common/Camera.hlsli"

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
    if (mode == 1 || mode == 2)
        return color;

    float3 normal = normalize(input.worldNormal);
    float3 lightDir = normalize(float3(0.5, 1.0, 0.3));
    float3 viewDir = normalize(cameraPos - input.worldPos);

    float3 ambient = 0.25 * float3(1.0, 1.0, 1.0);
    float diff = max(dot(normal, lightDir), 0.0);
    float3 diffuse = diff * float3(0.85, 0.85, 0.85);

    float3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    float3 specular = 0.2 * spec * float3(1.0, 1.0, 1.0);

    float3 finalLight = (ambient + diffuse + specular) * color.rgb;
    return float4(finalLight, color.a);
}
