#ifndef WE_FOUNDATION_PROCEDURAL_SKY_HLSL
#define WE_FOUNDATION_PROCEDURAL_SKY_HLSL

#include "../Common/Color.hlsli"
#include "../Common/CameraBuffer.hlsli"

// Foundation sky: display-referred, no bloom/post. cameraPadding = skyDebugMode:
// 0 final, 1 sky gradient only, 2 sun-disk mask, 3 luminance heatmap,
// 4 disable sun, 5 linear HDR (skip display tonemap / sRGB).

struct VSOutput
{
    float4 position : SV_Position;
    float3 viewDir  : TEXCOORD0;
};

// Must match DirectionalLightData::direction (light *travel* direction).
static const float3 kLightTravelDir = float3(0.3, -0.8, 0.2);
// Sun sits opposite the travel vector (above the horizon for daytime fills).
static const float3 kSunDir = normalize(-kLightTravelDir);
static const float3 kSunTint = float3(1.0, 0.96, 0.88);
// ~0.53° solar angular radius; small epsilon keeps the core resolvable.
static const float kSunCosCore = cos(radians(0.53));
static const float kSunCosCorona = cos(radians(2.5));

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

float3 SkyGradient(float3 dir)
{
    float height = saturate(dir.y * 0.5 + 0.5);
    // Mild, display-referred sky (kept well under 1.0 so the sun never washes the dome).
    float3 horizon = float3(0.55, 0.62, 0.72);
    float3 zenith  = float3(0.12, 0.28, 0.58);
    float3 nadir   = float3(0.22, 0.24, 0.26);
    float3 upper = lerp(horizon, zenith, pow(height, 1.25));
    return lerp(nadir, upper, saturate(dir.y * 0.85 + 0.85));
}

float3 EvaluateSun(float3 dir)
{
    float cosAngle = saturate(dot(dir, kSunDir));

    // Sharp solar disk.
    float disk = smoothstep(kSunCosCore - 0.00015, kSunCosCore + 0.00005, cosAngle);

    // Tight atmospheric aureole — NOT a wide bloom-like blur.
    float coronaT = saturate((cosAngle - kSunCosCorona) / max(kSunCosCore - kSunCosCorona, 1e-5));
    float corona = coronaT * coronaT * (3.0 - 2.0 * coronaT);
    corona = pow(corona, 2.5) * 0.35;

    return kSunTint * (disk * 2.5 + corona);
}

float3 EncodeDisplay(float3 linearColor, int debugMode)
{
    if (debugMode == 5)
        return saturate(linearColor);

    // Mild shoulder so bright areas stay stable without a full HDR post stack.
    float3 compressed = linearColor / (1.0 + linearColor * 0.35);
    return WE_LinearToSRGB(compressed);
}

float4 PSMain(VSOutput input) : SV_Target
{
    float3 dir = normalize(input.viewDir);
    int debugMode = (int)round(cameraPadding);

    float3 sky = SkyGradient(dir);
    float3 sun = EvaluateSun(dir);
    if (debugMode == 4)
        sun = 0.0;

    float3 linearColor = sky + sun;

    if (debugMode == 1)
        return float4(EncodeDisplay(sky, 0), 1.0);
    if (debugMode == 2)
    {
        float cosAngle = saturate(dot(dir, kSunDir));
        float disk = smoothstep(kSunCosCore - 0.00015, kSunCosCore + 0.00005, cosAngle);
        return float4(disk.xxx, 1.0);
    }
    if (debugMode == 3)
    {
        float lum = dot(linearColor, float3(0.2126, 0.7152, 0.0722));
        float3 heat = lerp(float3(0.0, 0.0, 0.3), float3(1.0, 0.9, 0.1), saturate(lum));
        heat = lerp(heat, float3(1.0, 0.2, 0.1), saturate(lum - 1.0));
        return float4(heat, 1.0);
    }

    return float4(EncodeDisplay(linearColor, debugMode), 1.0);
}

#endif
