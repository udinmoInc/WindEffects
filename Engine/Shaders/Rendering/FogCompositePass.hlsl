#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Rendering/SkyAtmosphere.hlsli"

cbuffer CameraBuffer : register(b0, space1)
{
    float4x4 view;
    float4x4 proj;
    float3   cameraPos;
    float    cameraPadding;
};

Texture2D    depthTexture : register(t0, space2);
SamplerState depthSampler : register(s0, space2);

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
    o.uv = uv;
    return o;
}

float3 WE_ReconstructWorldPos(float2 uv, float depth, float4x4 viewMat, float4x4 projMat, float3 camPos)
{
    float2 ndc = uv * 2.0 - 1.0;
    float4 clip = float4(ndc, depth, 1.0);
    float4x4 invView = WE_Inverse4x4(viewMat);
    float4x4 invProj = WE_Inverse4x4(projMat);
    float4 world = mul(invView, mul(invProj, clip));
    return world.xyz / world.w;
}

float4 PSMain(VSOutput input) : SV_Target
{
    if (enableVolumetricFog < 0.5)
        discard;

    const float depth = depthTexture.Sample(depthSampler, input.uv).r;
    if (depth < 1e-5)
        discard;

    const float3 worldPos = WE_ReconstructWorldPos(input.uv, depth, view, proj, cameraPos);
    const float3 relPos = worldPos - worldOrigin;
    const float3 relCam = cameraPos - worldOrigin;

    const float height = max(relPos.y, 0.0);
    const float heightFog = 1.0 - exp(-fogDensity * height * fogHeightFalloff);
    const float dist = length(relCam - relPos);
    const float distFog = 1.0 - exp(-max(dist - fogStartDistance, 0.0) * fogDensity * 0.4);
    const float fogFactor = saturate(heightFog * 0.55 + distFog * 0.45);

    const float3 viewDir = normalize(relCam - relPos);
    const float3 sunLinear = WE_sRGBToLinear(saturate(sunColor));
    const float3 rayleigh = max(atmosphereRayleigh, float3(1e-6, 1e-6, 1e-6));
    const float3 ozone = max(ozoneAbsorption, float3(0.0, 0.0, 0.0));

    // Fog inscattering matches horizon sky color for seamless blending.
    const float3 horizonSky = WE_SampleSkyAtmosphere(
        normalize(lerp(viewDir, float3(0.0, 0.0, 1.0), 0.35)), sunDirection,
        cameraPos, worldOrigin,
        sunLinear, sunIntensity * 0.5,
        rayleigh, mieScattering, ozone, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude);

    const float3 fogLinear = lerp(WE_sRGBToLinear(saturate(fogColor)), horizonSky, 0.65);
    const float ev = WE_ComputeExposureEV(sunDirection, exposureEV, exposureCompensation);
    float3 fogMapped = WE_ApplyFilmicTonemap(fogLinear, WE_ExposureFromEV100(ev));
    fogMapped = WE_LinearToSRGB(fogMapped);

    return float4(fogMapped, fogFactor);
}
