#define WE_CAMERA_BUFFER_SPACE space0
#include "../Common/CameraBuffer.hlsli"
#include "../Common/Color.hlsli"

// Lightweight editor landscape shading — same sun/sky terms as EnvironmentBuffer / BasicMesh.
cbuffer TerrainMaterial : register(b0, space1)
{
    float4 albedoColor;        // base RGB (sRGB authoring) + opacity
    float4 lightDirPad;        // xyz unused (kept for layout), w = roughness
    float4 materialPad;        // x = metallic, y = lightingValid, zw unused
    float4 gridParams;         // x = spacing (m), y = line width, z = opacity, w unused
    float4 gridColorPad;       // rgb = grid color, w unused
    float4 gridFadePad;        // x = fade start (m), y = fade end (m)
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

float WorldGridMask(float2 worldXZ, float spacing, float lineWidth)
{
    const float cell = max(spacing, 1e-3);
    const float2 uv = worldXZ / cell;
    const float2 grid = abs(frac(uv - 0.5) - 0.5);
    const float px = clamp(lineWidth, 0.35, 4.0);
    const float2 derivative = max(fwidth(uv) * px, float2(1e-4, 1e-4));
    const float lineX = 1.0 - smoothstep(0.0, derivative.x, grid.x);
    const float lineY = 1.0 - smoothstep(0.0, derivative.y, grid.y);
    return saturate(max(lineX, lineY));
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 albedo = WE_sRGBToLinear(saturate(albedoColor.rgb));
    const float3 normal = normalize(input.worldNormal);
    const float metallic = saturate(materialPad.x);
    const bool lightingValid = materialPad.y > 0.5;

    // Grid is expensive (fwidth) — skip entirely when opacity is off / tiny.
    const float gridOpacity = saturate(gridParams.z);
    if (gridOpacity > 0.02)
    {
        const float dist = length(cameraPos - input.worldPos);
        const float fadeEnd = max(gridFadePad.y, gridFadePad.x + 1.0);
        if (dist < fadeEnd)
        {
            const float lineMask = WorldGridMask(
                input.worldPos.xz, max(gridParams.x, 0.01), max(gridParams.y, 0.01));
            if (lineMask > 1e-3)
            {
                const float fadeStart = max(gridFadePad.x, 0.0);
                const float distanceFade = 1.0 - smoothstep(fadeStart, fadeEnd, dist);
                const float3 gridRgb = WE_sRGBToLinear(saturate(gridColorPad.rgb));
                albedo = lerp(albedo, gridRgb, saturate(lineMask * gridOpacity * distanceFade));
            }
        }
    }

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

    // Cheap specular (no fresnel / high pow).
    const float3 viewDir = normalize(cameraPos - input.worldPos);
    const float3 halfV = normalize(L + viewDir);
    const float spec = pow(saturate(dot(normal, halfV)), 32.0) * 0.04;
    const float3 specular = sunColor * sunIntensity * spec;

    float3 lit = ambient + diffuse + specular;
    lit = lit / (1.0 + lit * 0.35);
    return float4(WE_LinearToSRGB(lit), albedoColor.a);
}
