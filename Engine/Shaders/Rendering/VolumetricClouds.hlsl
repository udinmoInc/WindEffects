#ifndef WE_VOLUMETRIC_CLOUDS_HLSL
#define WE_VOLUMETRIC_CLOUDS_HLSL

// Unified volumetric pass entry — clouds + height fog + local fog share one integrator.
// Cloud morphology remains in VolumetricClouds.hlsli / CloudDensity provider.

#include "../Common/Color.hlsli"
#include "../Common/CameraBuffer.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Common/DaylightConfig.hlsli"
#include "Volumetrics/VolumeSample.hlsli"
#include "Volumetrics/VolumetricLighting.hlsli"
#include "Volumetrics/VolumetricShadow.hlsli"
#include "Volumetrics/VolumetricTemporal.hlsli"
#include "Volumetrics/VolumetricIntegrate.hlsli"
#include "Volumetrics/Providers/CloudDensity.hlsli"
#include "Volumetrics/Providers/HeightFogDensity.hlsli"
#include "Volumetrics/Providers/LocalFogDensity.hlsli"
#include "Volumetrics/Providers/AtmosphereDensity.hlsli"

// Descriptor sets: Camera=0, Env=1, Frame=2, Cloud=3, Resources=4
#ifndef WE_VOLUMETRIC_FRAME_SPACE
#define WE_VOLUMETRIC_FRAME_SPACE space2
#endif
#ifndef WE_CLOUD_BUFFER_SPACE
#define WE_CLOUD_BUFFER_SPACE space3
#endif
#ifndef WE_VOLUMETRIC_RESOURCE_SPACE
#define WE_VOLUMETRIC_RESOURCE_SPACE space4
#endif

cbuffer VolumetricFrameBuffer : register(b0, WE_VOLUMETRIC_FRAME_SPACE)
{
    uint   volMaxSteps;
    uint   volLightSteps;
    uint   volShadowSteps;
    uint   volFrameIndex;

    float  volResolutionScale;
    float  volTemporalBlend;
    float  volJitterX;
    float  volJitterY;

    uint   volProviderMask;
    uint   volTemporalQuality;
    uint   volShadowQuality;
    uint   volLightingQuality;

    float  volHeightFogDensity;
    float  volHeightFogFalloff;
    float  volHeightFogStart;
    float  volHeightFogEnabled;

    float3 volLocalFogCenter;
    float  volLocalFogDensity;

    float3 volLocalFogHalfExtents;
    float  volLocalFogAnisotropy;

    float3 volLocalFogAlbedo;
    float  volLocalFogEnabled;

    float4 volReserved;
};

// Must match CloudUniform (160 bytes).
cbuffer CloudBuffer : register(b0, WE_CLOUD_BUFFER_SPACE)
{
    float  cloudPlanetRadius;
    float  cloudInnerOffset;
    float  cloudOuterOffset;
    float  cloudCoverage;

    float  cloudType;
    float  cloudPrecipitation;
    float  cloudTime;
    float  cloudDensityScale;

    float3 cloudWindDir;
    float  cloudWindSpeed;

    float  cloudBaseScale;
    float  cloudDetailScale;
    float  cloudCurlStrength;
    float  cloudAbsorption;

    float  cloudAmbient;
    float  cloudSilverIntensity;
    float  cloudSilverSpread;
    float  cloudPowderStrength;

    float  cloudPhaseGForward;
    float  cloudPhaseGBack;
    uint   cloudMaxSteps;
    uint   cloudLightSteps;

    float  cloudEnabled;
    float  cloudLightMarchLength;
    float  cloudAlbedo;
    float  cloudWeatherMapScale;

    float  cloudErosionStrength;
    float  cloudBottomWispStrength;
    float  cloudBottomDensityReduce;
    float  cloudTypeWeightSharpness;

    float  cloudBaseWorleyInfluence;
    float  cloudCoverageRemapLow;
    float  cloudCoverageRemapHigh;
    float  cloudPrecipCbBlend;

    float  cloudFormationFreqLarge;
    float  cloudFormationFreqMedium;
    float  cloudFormationFreqSmall;
    float  cloudFormationThreshold;
};

Texture2D    depthTexture   : register(t0, WE_VOLUMETRIC_RESOURCE_SPACE);
Texture3D    baseShapeTex   : register(t1, WE_VOLUMETRIC_RESOURCE_SPACE);
SamplerState cloudSampler   : register(s1, WE_VOLUMETRIC_RESOURCE_SPACE);
Texture3D    detailShapeTex : register(t2, WE_VOLUMETRIC_RESOURCE_SPACE);
Texture2D    weatherMapTex  : register(t3, WE_VOLUMETRIC_RESOURCE_SPACE);
Texture2D    curlNoiseTex   : register(t4, WE_VOLUMETRIC_RESOURCE_SPACE);

static const uint WE_VOL_MASK_CLOUD      = 1u << 0;
static const uint WE_VOL_MASK_HEIGHT_FOG = 1u << 1;
static const uint WE_VOL_MASK_LOCAL_FOG  = 1u << 2;
static const uint WE_VOL_MASK_ATMOSPHERE = 1u << 8;

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
    float2 ndc      : TEXCOORD1;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    float2 uv = float2((vertexId << 1) & 2, vertexId & 2);
    float2 clipXY = uv * float2(2.0, -2.0) + float2(-1.0, 1.0);

    VSOutput o;
    o.position = float4(clipXY, 0.0, 1.0);
    o.uv = float2(clipXY.x * 0.5 + 0.5, clipXY.y * 0.5 + 0.5);
    o.ndc = clipXY;
    return o;
}

float3 ReconstructWorldRayDir(float2 ndc)
{
    float4 world = mul(invViewProj, float4(ndc, 1.0, 1.0));
    return normalize(world.xyz / max(world.w, 1e-6) - cameraPos);
}

float SceneDistanceMeters(float2 uv, float rawDepth)
{
    if (rawDepth >= 0.9999)
        return 1e9;

    float2 ndc = WE_UvToNdc(uv);
    float4 clip = float4(ndc, rawDepth, 1.0);
    float4 viewPos = mul(WE_Inverse4x4(proj), clip);
    viewPos /= max(viewPos.w, 1e-6);
    float4 world = mul(WE_Inverse4x4(view), viewPos);
    world.xyz /= max(world.w, 1e-6);
    return length(world.xyz - cameraPos);
}

VolumeSample EvaluateAllProviders(
    float3 worldPos,
    float3 planetPos,
    float innerRadius,
    float outerRadius,
    float3 windOffset,
    bool expensive)
{
    VolumeSample total = EmptyVolumeSample();

    if ((volProviderMask & WE_VOL_MASK_ATMOSPHERE) != 0u && volReserved.x >= 0.5)
    {
        VolumeSample atmosphere = EvaluateAtmosphereVolumeSample(
            worldPos, worldOrigin, planetRadius, atmosphereHeight,
            atmosphereRayleigh, mieScattering, 1.0);
        total = AccumulateVolumeSample(total, atmosphere);
    }

    if ((volProviderMask & WE_VOL_MASK_CLOUD) != 0u && cloudEnabled >= 0.5)
    {
        VolumeSample cloud = EvaluateCloudVolumeSample(
            planetPos, innerRadius, outerRadius,
            cloudCoverage, cloudType, cloudPrecipitation,
            windOffset, cloudBaseScale, cloudDetailScale, cloudCurlStrength,
            cloudDensityScale, cloudAbsorption, cloudAlbedo, cloudPhaseGForward,
            expensive,
            baseShapeTex, detailShapeTex, curlNoiseTex, cloudSampler);
        total = AccumulateVolumeSample(total, cloud);
    }

    if ((volProviderMask & WE_VOL_MASK_HEIGHT_FOG) != 0u)
    {
        VolumeSample fog = EvaluateHeightFogSample(
            worldPos, cameraPos,
            volHeightFogDensity, volHeightFogFalloff, volHeightFogStart, volHeightFogEnabled);
        total = AccumulateVolumeSample(total, fog);
    }

    if ((volProviderMask & WE_VOL_MASK_LOCAL_FOG) != 0u)
    {
        VolumeSample localFog = EvaluateLocalFogSample(
            worldPos, volLocalFogCenter, volLocalFogHalfExtents,
            volLocalFogDensity, volLocalFogAnisotropy, volLocalFogEnabled);
        localFog.scattering *= max(dot(volLocalFogAlbedo, float3(0.333, 0.333, 0.333)), 0.1);
        total = AccumulateVolumeSample(total, localFog);
    }

    return total;
}

float SampleLightOpticalDepth(
    float3 planetPos,
    float3 worldBase,
    float3 toSun,
    float innerRadius,
    float outerRadius,
    float3 windOffset,
    bool cheap)
{
    const uint configured = max(max(cloudLightSteps, volLightSteps), 1u);
    const float stepBudget = WE_VolumetricShadowStepCount(configured, volShadowQuality);
    const uint steps = (uint)clamp(stepBudget, 1.0, 16.0);
    const float marchLen = min(max(cloudLightMarchLength, 100.0), 900.0);
    const float ds = marchLen / float(steps);

    float optical = 0.0;
    float3 pPlanet = planetPos;
    float3 pWorld = worldBase;
    [loop]
    for (uint i = 0; i < steps; ++i)
    {
        pPlanet += toSun * ds;
        pWorld += toSun * ds;
        const float h = CloudHeightFraction(pPlanet, innerRadius, outerRadius);
        // Keep marching fog even outside cloud shell.
        VolumeSample s = EvaluateAllProviders(
            pWorld, pPlanet, innerRadius, outerRadius, windOffset, !cheap);
        if (s.density <= 1e-5 && (h <= 0.0 || h >= 1.0))
            break;
        optical += s.density * ds;
    }
    return optical;
}

float4 PSMain(VSOutput input) : SV_Target
{
    if (volProviderMask == 0u)
        discard;

    const float3 rayDir = ReconstructWorldRayDir(input.ndc);
    const float planetRadius = max(cloudPlanetRadius, 1.0);
    const float innerRadius = planetRadius + max(cloudInnerOffset, 1.0);
    const float outerRadius = planetRadius + max(cloudOuterOffset, cloudInnerOffset + 1.0);
    const float3 planetCam = WorldToPlanetSpace(cameraPos, worldOrigin, planetRadius);

    const uint2 pix = uint2(input.position.xy);
    const float rawDepth = depthTexture.Load(int3(pix, 0)).r;
    const float sceneDist = SceneDistanceMeters(input.uv, rawDepth);

    float tStart = 0.0;
    float tEnd = min(sceneDist, WE_CLOUD_MAX_MARCH_M);
    bool hasInterval = false;

    if ((volProviderMask & WE_VOL_MASK_CLOUD) != 0u && cloudEnabled >= 0.5)
    {
        float cloudStart = 0.0;
        float cloudEnd = 0.0;
        if (CloudShellIntersect(
                planetCam, rayDir, planetRadius, innerRadius, outerRadius, cloudStart, cloudEnd))
        {
            tStart = cloudStart;
            tEnd = min(cloudEnd, tEnd);
            hasInterval = true;
        }
    }

    // Fog / atmosphere providers march from near camera to scene depth when clouds miss.
    // Cap air-only distance so media stays aerial-perspective (never a full opaque sky wipe).
    if ((volProviderMask & (WE_VOL_MASK_HEIGHT_FOG | WE_VOL_MASK_LOCAL_FOG | WE_VOL_MASK_ATMOSPHERE)) != 0u)
    {
        if (!hasInterval)
        {
            const float airMax = 2500.0;
            tStart = 0.0;
            tEnd = min(sceneDist, airMax);
            hasInterval = tEnd > 1.0;
        }
        else
        {
            tStart = min(tStart, 0.0);
        }
    }

    if (!hasInterval || tStart >= tEnd || tEnd <= 0.0)
        discard;

    if (rayDir.y < 0.02 && sceneDist < 2000.0
        && (volProviderMask & WE_VOL_MASK_CLOUD) != 0u
        && (volProviderMask & (WE_VOL_MASK_HEIGHT_FOG | WE_VOL_MASK_LOCAL_FOG | WE_VOL_MASK_ATMOSPHERE)) == 0u)
        discard;

    const float3 windDirN = length(cloudWindDir) > 1e-4
        ? normalize(cloudWindDir)
        : float3(1.0, 0.0, 0.0);
    const float3 windOffset = windDirN * (cloudTime * cloudWindSpeed);

    const uint maxSteps = clamp(max(cloudMaxSteps, volMaxSteps), 24u, 256u);
    const float marchLen = max(tEnd - tStart, 1.0);
    const float minStep = 16.0;
    const float maxStep = 95.0;
    float stepSize = clamp(marchLen / float(maxSteps), minStep, maxStep);

    const float jitter = WE_VolumetricRayJitter(input.position.xy, volJitterX, volJitterY);
    float t = tStart + jitter * stepSize;

    const float3 toSun = normalize(-sunDirection);
    const float lightAbsorption = clamp(cloudAbsorption, 0.045, 0.12);
    const float viewAbsorption = clamp(max(cloudAbsorption * 3.0, 0.20), 0.16, 0.36);

    float3 accumulatedColor = 0.0.xxx;
    float transmittance = 1.0;
    float meanDist = 0.0;
    float meanWeight = 0.0;

    uint stepCount = 0;
    const uint stepBudget = maxSteps + (maxSteps / 2u);
    const float densityEpsilon = 0.001;

    // Silence unused weather map binding warning in some compilers.
    const float weatherProbe = weatherMapTex.SampleLevel(cloudSampler, float2(0.5, 0.5), 0.0).r;
    (void)weatherProbe;
    (void)cloudWeatherMapScale;
    (void)cloudFormationFreqLarge;
    (void)cloudFormationFreqMedium;
    (void)cloudFormationFreqSmall;
    (void)cloudFormationThreshold;
    (void)cloudErosionStrength;
    (void)cloudBottomWispStrength;
    (void)cloudBottomDensityReduce;
    (void)cloudTypeWeightSharpness;
    (void)cloudBaseWorleyInfluence;
    (void)cloudCoverageRemapLow;
    (void)cloudCoverageRemapHigh;
    (void)cloudPrecipCbBlend;
    (void)volLightingQuality;
    (void)volResolutionScale;

    [loop]
    while (t < tEnd && transmittance > 0.01 && stepCount < stepBudget)
    {
        ++stepCount;
        const float3 planetPos = planetCam + rayDir * t;
        const float3 worldPos = cameraPos + rayDir * t;

        VolumeSample sample = EvaluateAllProviders(
            worldPos, planetPos, innerRadius, outerRadius, windOffset, true);

        float ds = stepSize;
        if (sample.density <= densityEpsilon)
        {
            ds = min(stepSize * 2.0, maxStep * 1.4);
            t += ds;
            continue;
        }

        ds = clamp(lerp(stepSize * 0.65, stepSize, saturate(1.0 - sample.density * 60.0)),
                   minStep * 0.7, maxStep);

        const bool lightCheap = (transmittance < 0.45);
        const float optical = SampleLightOpticalDepth(
            planetPos, worldPos, toSun, innerRadius, outerRadius, windOffset, lightCheap);

        const float cosTheta = dot(rayDir, toSun);
        const float heightFraction = CloudHeightFraction(planetPos, innerRadius, outerRadius);

        const bool cloudMedia =
            (volProviderMask & WE_VOL_MASK_CLOUD) != 0u && cloudEnabled >= 0.5;
        float3 lightRGB;
        if (cloudMedia)
        {
            VolumetricLightResult lighting = WE_EvaluateVolumetricLighting(
                rayDir, toSun, sunColor, sunIntensity, skyAmbientColor, skyLightIntensity,
                cloudAmbient, heightFraction, optical, lightAbsorption, cosTheta,
                cloudPowderStrength, cloudPhaseGForward, cloudPhaseGBack,
                cloudSilverIntensity, cloudSilverSpread, cloudAlbedo, multiScatterStrength);
            lightRGB = lighting.direct + lighting.ambient + lighting.multiScatter;
        }
        else
        {
            // Fog / air media: light from shared EnvironmentLighting (same as ProceduralSky).
            WE_EnvLightSample envL = WE_EvalEnvironmentLighting(rayDir);
            lightRGB = envL.skyRadiance * 0.90 + envL.sunRadiance * 0.08;
            lightRGB = max(lightRGB, max(fogColor, 0.0.xxx) * max(skyLightIntensity, 0.35));
        }
        if ((volProviderMask & WE_VOL_MASK_LOCAL_FOG) != 0u && volLocalFogEnabled >= 0.5)
            lightRGB = lerp(lightRGB, lightRGB * volLocalFogAlbedo, 0.35);

        WE_AccumulateVolumeStep(
            sample, ds, viewAbsorption, lightRGB, accumulatedColor, transmittance);

        const float scatterWeight = 1.0 - transmittance;
        meanDist += t * max(sample.density * ds, 0.0);
        meanWeight += max(sample.density * ds, 0.0);

        if (transmittance < 0.01)
        {
            transmittance = 0.0;
            break;
        }
        t += ds;
    }

    const float opticalAlpha = saturate(1.0 - transmittance);
    if (opticalAlpha < 0.008)
        discard;

    const float3 toSunN = normalize(-sunDirection);
    WE_EnvLightSample envSky = WE_EvalEnvironmentLighting(rayDir);
    const float3 skyBg = max(envSky.skyRadiance, ApproximateSkyRadiance(
        rayDir, toSunN,
        max(sunColor, 0.0.xxx) * max(sunIntensity, 0.0),
        max(skyAmbientColor, 0.0.xxx) * max(skyLightIntensity, 0.0) * cloudAmbient * 0.32));

    const float cloudDist = (meanWeight > 1e-4)
        ? (meanDist / meanWeight)
        : (tStart + marchLen * 0.5);

    const float horizonFade = saturate(
        (cloudDist - WE_CLOUD_HORIZON_FADE_START_M) /
        max(WE_CLOUD_HORIZON_FADE_END_M - WE_CLOUD_HORIZON_FADE_START_M, 1.0));
    const float distanceExtinction = exp(-cloudDist * WE_CLOUD_ATMO_EXTINCTION);
    const float elev = max(rayDir.y, 0.0);
    const float geometricHorizon = saturate(1.0 - elev * 4.0);

    float3 cloudColor = accumulatedColor;
    const float haze = saturate((1.0 - distanceExtinction) * 0.12 + horizonFade * 0.08)
        * geometricHorizon;
    cloudColor = lerp(cloudColor, skyBg * opticalAlpha, haze);

    float alpha = saturate(opticalAlpha);
    alpha *= lerp(1.0, 0.85, horizonFade * geometricHorizon);

    // Air-only media must preserve ProceduralSky — never SceneColor = VolumetricColor.
    const bool airOnly =
        ((volProviderMask & WE_VOL_MASK_CLOUD) == 0u || cloudEnabled < 0.5);
    if (airOnly)
    {
        alpha = min(alpha, 0.55);
        // Lift near-black accumulation to environment sky so fog reads as haze.
        const float3 safeLit = max(cloudColor, skyBg * alpha * 0.65);
        cloudColor = lerp(skyBg * alpha, safeLit, 0.85);
    }

    // Match ProceduralSky display encode (viewport is sampled as display-ready today).
    const float exposureScale = exp2(exposureEV + exposureCompensation * 0.25);
    const float3 unpremult = cloudColor / max(alpha, 1e-3);
    const float3 encoded = WE_LinearToSRGB(WE_EncodeDaylight(max(unpremult * max(exposureScale, 1e-4), 0.0)));
    float4 current = float4(encoded * alpha, alpha);
    // History sampling stub — VolumetricRenderer owns history targets; blend identity for now.
    current = WE_VolumetricTemporalAccumulate(current, current, volTemporalBlend, volTemporalQuality);

    if (current.a < 0.008)
        discard;

    return current;
}

#endif // WE_VOLUMETRIC_CLOUDS_HLSL
