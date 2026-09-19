#ifndef WE_VOLUMETRIC_CLOUDS_HLSL
#define WE_VOLUMETRIC_CLOUDS_HLSL

// Fullscreen volumetric cloud ray-marcher — Horizon Zero Dawn / Nubis architecture.
// World units: meters (matches terrain / editor camera).

#include "../Common/Color.hlsli"
#include "../Common/CameraBuffer.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "VolumetricClouds.hlsli"

#ifndef WE_CLOUD_BUFFER_SPACE
#define WE_CLOUD_BUFFER_SPACE space2
#endif

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
    float  cloudPadding;
};

Texture2D    depthTexture   : register(t0, space3);
Texture3D    baseShapeTex   : register(t1, space3);
SamplerState cloudSampler   : register(s1, space3);
Texture3D    detailShapeTex : register(t2, space3);
Texture2D    curlNoiseTex   : register(t3, space3);

struct VSOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
    float2 ndc      : TEXCOORD1;
};

VSOutput VSMain(uint vertexId : SV_VertexID)
{
    // Fullscreen triangle — only pass linear NDC/UV. Do NOT interpolate viewDir:
    // normalize(unproject(ndc)) is nonlinear, so VS interpolation tilts the
    // world-Y cloud slab across the screen while the landscape stays level.
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
    // Same far-plane unproject as ProceduralSky (clip NDC, not UV).
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

float CloudRayJitter(float2 pixel)
{
    const float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(magic.z * frac(dot(pixel, magic.xy)));
}

float SampleLightOpticalDepth(
    float3 planetPos,
    float3 toSun,
    float innerRadius,
    float outerRadius,
    float3 windOffset,
    bool cheap)
{
    const uint steps = max(cloudLightSteps, 1u);
    // Cap light path so beer does not slam to zero through thick packs.
    const float marchLen = min(max(cloudLightMarchLength, 100.0), 900.0);
    const float ds = marchLen / float(steps);

    float optical = 0.0;
    float3 p = planetPos;
    [loop]
    for (uint i = 0; i < steps; ++i)
    {
        p += toSun * ds;
        // World-Y shell (matches density sample) — not planet-radial length().
        const float h = CloudHeightFractionY(p, cloudPlanetRadius, innerRadius, outerRadius);
        if (h <= 0.0 || h >= 1.0)
            break;

        CloudDensityResult d = SampleCloudDensity(
            p, innerRadius, outerRadius,
            cloudCoverage, cloudType, cloudPrecipitation,
            windOffset, cloudBaseScale, cloudDetailScale, cloudCurlStrength,
            !cheap,
            baseShapeTex, detailShapeTex, curlNoiseTex, cloudSampler);
        optical += d.density * cloudDensityScale * ds;
    }
    return optical;
}

float4 PSMain(VSOutput input) : SV_Target
{
    if (cloudEnabled < 0.5)
        discard;

    // Per-pixel ray from interpolated NDC (linear). Never interpolate viewDir.
    const float3 rayDir = ReconstructWorldRayDir(input.ndc);

    // Atmosphere params are meters — engine world space is meters (not cm).
    const float planetRadius = max(cloudPlanetRadius, 1.0);
    const float innerRadius = planetRadius + max(cloudInnerOffset, 1.0);
    const float outerRadius = planetRadius + max(cloudOuterOffset, cloudInnerOffset + 1.0);

    const float3 planetCam = WorldToPlanetSpace(cameraPos, worldOrigin, planetRadius);

    // Hard early-out only when looking into nearby ground (world -Y).
    const uint2 pix = uint2(input.position.xy);
    const float rawDepth = depthTexture.Load(int3(pix, 0)).r;
    const float sceneDist = SceneDistanceMeters(input.uv, rawDepth);

    if (rayDir.y < 0.02 && sceneDist < 2000.0)
        discard;

    float tStart = 0.0;
    float tEnd = 0.0;
    if (!CloudShellIntersect(
            planetCam, rayDir, planetRadius, innerRadius, outerRadius, tStart, tEnd))
        discard;

    tEnd = min(tEnd, sceneDist);
    if (tStart >= tEnd || tEnd <= 0.0)
        discard;

    const float3 windDirN = length(cloudWindDir) > 1e-4
        ? normalize(cloudWindDir)
        : float3(1.0, 0.0, 0.0);
    const float3 windOffset = windDirN * (cloudTime * cloudWindSpeed);

    const uint maxSteps = clamp(cloudMaxSteps, 24u, 192u);
    const float marchLen = max(tEnd - tStart, 1.0);

    const float minStep = 16.0;
    const float maxStep = 95.0;
    float stepSize = clamp(marchLen / float(maxSteps), minStep, maxStep);

    const float jitter = CloudRayJitter(input.position.xy);
    float t = tStart + jitter * stepSize;

    const float3 toSun = normalize(-sunDirection);
    const float3 sunCol = max(sunColor, 0.0.xxx) * max(sunIntensity, 0.0);
    // Soft sky fill — tops stay bright via HeightAmbientFactor, not muddy blue.
    const float3 ambientCol = max(skyAmbientColor, 0.0.xxx)
        * max(skyLightIntensity, 0.0) * cloudAmbient * 0.32;
    const float3 skyBg = ApproximateSkyRadiance(rayDir, toSun, sunCol, ambientCol);
    const float lightAbsorption = clamp(cloudAbsorption, 0.045, 0.12);
    // Keep view body (don't ghost), but leave room for sunlit white.
    const float viewAbsorption = clamp(max(cloudAbsorption * 3.0, 0.20), 0.16, 0.36);

    float3 accumulatedColor = 0.0.xxx;
    float transmittance = 1.0;
    float meanDist = 0.0;
    float meanWeight = 0.0;

    uint stepCount = 0;
    const uint stepBudget = maxSteps + (maxSteps / 2u);
    // Empty-air gate: never accumulate ambient / extinction below this.
    const float densityEpsilon = 0.001;

    [loop]
    while (t < tEnd && transmittance > 0.01 && stepCount < stepBudget)
    {
        ++stepCount;
        const float3 planetPos = planetCam + rayDir * t;

        CloudDensityResult sample = SampleCloudDensity(
            planetPos, innerRadius, outerRadius,
            cloudCoverage, cloudType, cloudPrecipitation,
            windOffset, cloudBaseScale, cloudDetailScale, cloudCurlStrength,
            true,
            baseShapeTex, detailShapeTex, curlNoiseTex, cloudSampler);

        float density = sample.density * cloudDensityScale;
        float ds = stepSize;

        // CRITICAL: Skip empty air entirely.
        // Do NOT add ambient light, do NOT reduce transmittance if density <= epsilon.
        if (density <= densityEpsilon)
        {
            ds = min(stepSize * 2.0, maxStep * 1.4);
            t += ds;
            continue;
        }

        ds = clamp(lerp(stepSize * 0.65, stepSize, saturate(1.0 - density * 60.0)),
                   minStep * 0.7, maxStep);

        const bool lightCheap = (transmittance < 0.45);
        const float optical = SampleLightOpticalDepth(
            planetPos, toSun, innerRadius, outerRadius, windOffset, lightCheap);

        const float cosTheta = dot(rayDir, toSun);
        // Schneider beer-powder (2 * beer * powder) + multi-scatter core fill.
        const float beerPowder = BeerPowderMultiScatter(
            optical, lightAbsorption, cosTheta, cloudPowderStrength);
        const float phase = DualLobeHG(cosTheta, cloudPhaseGForward, cloudPhaseGBack, 0.55);
        const float silver = SilverLining(cosTheta, cloudSilverIntensity, cloudSilverSpread);

        const float coreFill = saturate(1.0 - BeerLaw(optical, lightAbsorption));
        const float3 ambient = ambientCol
            * HeightAmbientFactor(sample.heightFraction)
            * (1.0 + coreFill * 0.20);

        const float directLight = beerPowder * phase * silver;
        // Slight warm lift so lit faces read white-sun, not cool mud.
        float3 sunScatter = sunCol * max(directLight, 0.0) * float3(1.05, 1.02, 0.98);
        sunScatter = sunScatter / (1.0 + sunScatter * 0.18);

        const float3 stepScattering =
            (sunScatter + ambient) * density * cloudAlbedo;

        const float extinctionCoeff = max(viewAbsorption, 1e-4);
        const float stepExtinction = exp(-density * ds * extinctionCoeff);
        const float3 stepIntegral =
            stepScattering * ((1.0 - stepExtinction) / max(density * extinctionCoeff, 1e-6));
        accumulatedColor += stepIntegral * transmittance;
        transmittance *= stepExtinction;

        const float scatterWeight = 1.0 - stepExtinction;
        meanDist += t * scatterWeight;
        meanWeight += scatterWeight;

        // Early exit when cloud becomes completely opaque.
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

    const float cloudDist = (meanWeight > 1e-4)
        ? (meanDist / meanWeight)
        : (tStart + marchLen * 0.5);

    const float horizonFade = saturate(
        (cloudDist - WE_CLOUD_HORIZON_FADE_START_M) /
        max(WE_CLOUD_HORIZON_FADE_END_M - WE_CLOUD_HORIZON_FADE_START_M, 1.0));
    const float distanceExtinction = exp(-cloudDist * WE_CLOUD_ATMO_EXTINCTION);
    const float elev = max(rayDir.y, 0.0);
    const float geometricHorizon = saturate(1.0 - elev * 4.0);

    // Premultiplied radiance already integrated — do not divide by alpha or force
    // a white floor (that bleaches empty-air / thin-cloud pixels).
    float3 cloudColor = accumulatedColor;
    const float haze = saturate((1.0 - distanceExtinction) * 0.12 + horizonFade * 0.08)
        * geometricHorizon;
    cloudColor = lerp(cloudColor, skyBg * opticalAlpha, haze);

    float alpha = saturate(opticalAlpha);
    alpha *= lerp(1.0, 0.85, horizonFade * geometricHorizon);

    if (alpha < 0.008)
        discard;

    return float4(cloudColor, alpha);
}

#endif // WE_VOLUMETRIC_CLOUDS_HLSL
