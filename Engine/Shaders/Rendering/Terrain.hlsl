#define WE_CAMERA_BUFFER_SPACE space0
#include "../Common/CameraBuffer.hlsli"
#include "../Common/Color.hlsli"

// Landscape PBR shading — checker placeholder comes from material params, never editor grid.
cbuffer TerrainMaterial : register(b0, space1)
{
    float4 albedoColor;        // base RGB (sRGB authoring) + opacity
    float4 lightDirPad;        // xyz unused (layout), w = roughness
    float4 materialPad;        // x = metallic, y = lightingValid, z = specular
    float4 checkerParams;      // x = cell size (m), y = useChecker (0/1)
    float4 checkerColorA;      // rgb = light checker square
    float4 checkerColorB;      // rgb = dark checker square
    float4 sunTravelPad;       // xyz = sun travel direction, w = sunIntensity
    float4 sunColorPad;        // rgb = sunColor
    float4 skyAmbientPad;      // rgb = skyAmbientColor, w = skyLightIntensity
    float4 skyLowerPad;        // rgb = skyLightLowerColor
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
};

VSOutput VSMain(VSInput input)
{
    VSOutput o;
    o.worldPos = input.position;
    o.worldNormal = input.normal;
    o.position = mul(proj, mul(view, float4(input.position, 1.0)));
    return o;
}

float3 WorldCheckerAlbedo(float2 worldXZ, float cellSize, float3 colorA, float3 colorB)
{
    const float cell = max(cellSize, 1e-3);
    const float2 uv = worldXZ / cell;
    const int2 cellId = int2(floor(uv));
    const float checker = ((cellId.x + cellId.y) & 1) ? 1.0 : 0.0;
    return lerp(colorA, colorB, checker);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 albedo = WE_sRGBToLinear(saturate(albedoColor.rgb));
    if (checkerParams.y > 0.5)
    {
        const float3 colorA = WE_sRGBToLinear(saturate(checkerColorA.rgb));
        const float3 colorB = WE_sRGBToLinear(saturate(checkerColorB.rgb));
        albedo = WorldCheckerAlbedo(input.worldPos.xz, max(checkerParams.x, 0.01), colorA, colorB);
    }

    const float3 normal = normalize(input.worldNormal);
    const float roughness = saturate(max(lightDirPad.w, 0.04));
    const float metallic = saturate(materialPad.x);
    const float specularScale = saturate(materialPad.z);
    const bool lightingValid = materialPad.y > 0.5;

    float3 sunTravel = lightingValid ? sunTravelPad.xyz : float3(0.3, -0.8, 0.2);
    float sunIntensity = lightingValid ? max(sunTravelPad.w, 0.0) : 1.2;
    float3 sunColor = lightingValid ? max(sunColorPad.rgb, 0.0) : float3(1.0, 0.98, 0.95);
    float skyIntensity = lightingValid ? max(skyAmbientPad.w, 0.0) : 1.0;
    float3 skyUpper = lightingValid ? max(skyAmbientPad.rgb, 0.0) : float3(0.35, 0.42, 0.55);
    float3 skyLower = lightingValid ? max(skyLowerPad.rgb, 0.0) : float3(0.15, 0.16, 0.18);

    const float3 L = normalize(-sunTravel);
    const float ndotl = saturate(dot(normal, L));
    const float hemi = saturate(normal.y * 0.5 + 0.5);
    const float3 ambient = albedo * lerp(skyLower, skyUpper, hemi) * skyIntensity;
    const float3 diffuse = albedo * sunColor * sunIntensity * ndotl * (1.0 - metallic);

    const float3 viewDir = normalize(cameraPos - input.worldPos);
    const float3 halfV = normalize(L + viewDir);
    const float specPower = lerp(8.0, 128.0, 1.0 - roughness);
    const float spec = pow(saturate(dot(normal, halfV)), specPower) * specularScale * (1.0 - metallic);
    const float3 specular = sunColor * sunIntensity * spec;

    float3 lit = ambient + diffuse + specular;
    lit = lit / (1.0 + lit * 0.35);
    return float4(WE_LinearToSRGB(lit), albedoColor.a);
}
