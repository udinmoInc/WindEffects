#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Rendering/AtmosphereLUT.hlsli"

cbuffer CameraBuffer : register(b0, space1)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

Texture2D    depthTexture : register(t0, space2);
Texture2D    aerialLUT    : register(t1, space2);
Texture2D    skyViewLUT   : register(t2, space2);
SamplerState lutSampler   : register(s0, space2);

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float2 pos = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);
    VSOutput o;
    o.position = float4(pos, 0.999, 1.0);
    o.uv = uv * 0.5;
    return o;
}

float3 WE_ReconstructWorldPos(float2 uv, float depth, float4x4 viewMat, float4x4 projMat, float3 camPos)
{
    float2 ndc = uv * 2.0 - 1.0;
    float4 clip = float4(ndc, depth, 1.0);
    float4x4 invView = WE_Inverse4x4(viewMat);
    float4x4 invProj = WE_Inverse4x4(projMat);
    float4 viewPos = mul(invProj, clip);
    viewPos /= max(viewPos.w, 1e-6);
    float4 world = mul(invView, viewPos);
    return world.xyz / max(world.w, 1e-6);
}

float4 PSMain(VSOutput input) : SV_Target
{
    if (enableVolumetricFog < 0.5)
        discard;

    // Nearest-texel fetch — linear depth filtering smears geometry edges into the sky
    // and produces large dark fog halos (reverse-Z makes this especially visible).
    const uint2 pix = uint2(input.position.xy);
    const float depth = depthTexture.Load(int3(pix, 0)).r;
    // Reverse-Z: cleared depth is 0 (far); skip sky pixels with no geometry.
    if (depth < 1e-4)
        discard;

    const float3 worldPos = WE_ReconstructWorldPos(input.uv, depth, view, proj, cameraPos);
    const float3 relPos = worldPos - worldOrigin;
    const float3 relCam = cameraPos - worldOrigin;

    const float height = max(relPos.y, 0.0);
    const float heightFog = 1.0 - exp(-fogDensity * height * max(fogHeightFalloff, 0.01));
    const float dist = length(relCam - relPos);
    const float distFog = 1.0 - exp(-max(dist - fogStartDistance, 0.0) * fogDensity * 0.15);
    const float fogFactor = saturate(heightFog * 0.35 + distFog * 0.65);

    const float3 viewDir = normalize(relCam - relPos);
    const float3 rayleigh = max(atmosphereRayleigh, float3(1e-6, 1e-6, 1e-6));
    const float3 ozone = max(ozoneAbsorption, float3(0.0, 0.0, 0.0));

    const float3 aerial = WE_SampleAerialPerspective(
        viewDir, sunDirection, cameraPos, worldOrigin, worldPos,
        sunColor, sunIntensity,
        rayleigh, mieScattering, ozone, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        max(sunAngularRadius, WE_SUN_ANGULAR_RADIUS),
        aerialLUT, skyViewLUT, lutSampler);

    const float3 fogTint = WE_sRGBToLinear(saturate(fogColor));
    const float3 fogFloor = fogTint * sunIntensity * 0.06 + float3(0.02, 0.04, 0.07);
    const float3 fogLinear = max(lerp(fogFloor, aerial, 0.82), fogFloor);
    return float4(fogLinear, fogFactor * 0.45);
}
