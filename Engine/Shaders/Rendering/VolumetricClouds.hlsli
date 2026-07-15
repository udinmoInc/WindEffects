#ifndef WE_VOLUMETRIC_CLOUDS_HLSLI
#define WE_VOLUMETRIC_CLOUDS_HLSLI

#include "../Common/Noise.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"
#include "../Common/CloudFrameBuffer.hlsli"

float WE_CloudRemap(float value, float low, float high)
{
    return saturate((value - low) / max(high - low, 1e-4));
}

// Finite horizontal domain radius (meters) around worldOrigin.xz.
float WE_CloudDomainRadius()
{
    // Prefer frame UBO; fall back so volume never becomes infinite.
    return max(cloudDomainRadius, 1000.0);
}

// Soft radial mask: 1 in the interior, 0 at/ beyond the cylinder wall.
float WE_CloudRadialMask(float3 worldPos, float3 worldOrigin)
{
    const float2 d = worldPos.xz - worldOrigin.xz;
    const float r = length(d);
    const float radius = WE_CloudDomainRadius();
    const float edge = max(radius * 0.18, 200.0);
    return 1.0 - smoothstep(radius - edge, radius, r);
}

float WE_CloudWeatherCoverage(float3 worldPos)
{
    const float3 wind = normalize(cloudWindDir + float3(1e-4, 0.0, 0.0)) * (cloudWindSpeed * cloudAnimTime * 0.12);
    const float2 uv = (worldPos.xz + wind.xz) * (0.00085 * max(cloudNoiseScale, 0.05));
    const float large = WE_FBM3D(float3(uv.x, cloudSeed * 0.07, uv.y), 4);
    const float medium = WE_FBM3D(float3(uv.x, cloudSeed * 0.07, uv.y) * 2.4 + 11.0, 3);
    // Bias toward clearer sky — coverage is applied in density remap, not here.
    return saturate(large * 0.55 + medium * 0.35);
}

float WE_CloudShapeNoiseSample(float3 worldPos)
{
    const float3 wind = normalize(cloudWindDir + float3(1e-4, 0.0, 0.0)) * (cloudWindSpeed * cloudAnimTime);
    const float3 noisePos = (worldPos + wind) * (0.0032 * max(cloudNoiseScale, 0.05))
        + float3(cloudSeed, cloudAltitude * 0.001, cloudSeed * 0.37);
    return WE_FBM3D(noisePos, 5);
}

float WE_CloudDetailNoiseSample(float3 worldPos)
{
    const float3 wind = normalize(cloudWindDir + float3(1e-4, 0.0, 0.0)) * (cloudWindSpeed * cloudAnimTime * 1.35);
    const float3 noisePos = (worldPos + wind) * (0.0032 * max(cloudNoiseScale, 0.05) * max(cloudDetailScale, 1.0))
        + float3(cloudSeed + 19.7, 4.2, cloudSeed * 0.21);
    return WE_FBM3D(noisePos, 3);
}

float WE_CloudHeightProfile(float heightM)
{
    const float bottom = cloudBottomAltitude;
    const float top = max(cloudTopAltitude, bottom + 50.0);
    // Strictly zero outside the altitude slab.
    if (heightM < bottom || heightM > top)
        return 0.0;
    const float h = saturate((heightM - bottom) / max(top - bottom, 1.0));
    return saturate(1.0 - abs(h * 2.0 - 1.05) * 1.05)
        * smoothstep(0.0, 0.14, h)
        * smoothstep(1.0, 0.68, h);
}

float WE_CloudDensityAt(float3 worldPos, float3 worldOrigin)
{
    const float radial = WE_CloudRadialMask(worldPos, worldOrigin);
    if (radial <= 1e-4)
        return 0.0;

    const float heightProfile = WE_CloudHeightProfile(worldPos.y - worldOrigin.y);
    if (heightProfile <= 1e-4)
        return 0.0;

    const float weather = WE_CloudWeatherCoverage(worldPos);
    if (weather < cloudEmptySkipThreshold * 0.5)
        return 0.0;

    // Scattered cumulus: remapped coverage leaves clear blue-sky gaps.
    const float coverage = saturate(cloudCoverage);
    const float localCoverage = saturate(coverage * (0.25 + 0.75 * weather));
    if (localCoverage <= 0.08)
        return 0.0;

    const float shape = WE_CloudShapeNoiseSample(worldPos);
    const float detail = WE_CloudDetailNoiseSample(worldPos);

    // Harder coverage gate so density is only inside cumulus blobs, not a sky-wide haze.
    float dens = WE_CloudRemap(shape, 1.0 - localCoverage * 0.85, 1.0);
    dens = pow(dens, 1.35);
    dens *= saturate(cloudShapeNoise);
    dens = saturate(dens - detail * saturate(cloudErosionNoise) * (1.0 - dens * 0.35) * 0.7);
    return dens * heightProfile * radial * max(cloudDensityMult, 0.0);
}

float WE_CloudOccupancyHint(float3 worldPos, float3 worldOrigin)
{
    const float radial = WE_CloudRadialMask(worldPos, worldOrigin);
    if (radial <= 1e-4)
        return 0.0;
    const float heightProfile = WE_CloudHeightProfile(worldPos.y - worldOrigin.y);
    if (heightProfile <= 1e-4)
        return 0.0;
    const float weather = WE_CloudWeatherCoverage(worldPos);
    return weather * heightProfile * radial * saturate(cloudCoverage);
}

float WE_HenyeyGreenstein(float cosTheta, float g)
{
    const float g2 = g * g;
    const float denom = pow(max(1.0 + g2 - 2.0 * g * cosTheta, 1e-4), 1.5);
    return (1.0 - g2) / (4.0 * 3.14159265 * denom);
}

// Intersect ray with a finite cloud volume: Y slab ∩ vertical cylinder.
// Marching is strictly limited to [tEnter, tExit] — never camera→far-plane.
bool WE_IntersectCloudVolume(
    float3 rayOrigin,
    float3 rayDir,
    float3 worldOrigin,
    out float tEnter,
    out float tExit)
{
    tEnter = 0.0;
    tExit = -1.0;
    rayDir = normalize(rayDir);

    const float bottom = worldOrigin.y + cloudBottomAltitude;
    const float top = worldOrigin.y + max(cloudTopAltitude, cloudBottomAltitude + 50.0);

    // --- Y-slab intersection ---
    float tY0;
    float tY1;
    if (abs(rayDir.y) < 1e-6)
    {
        // Parallel to planes: only valid if already inside the slab.
        if (rayOrigin.y < bottom || rayOrigin.y > top)
            return false;
        tY0 = 0.0;
        tY1 = cloudMaxMarchDistance;
    }
    else
    {
        tY0 = (bottom - rayOrigin.y) / rayDir.y;
        tY1 = (top - rayOrigin.y) / rayDir.y;
        if (tY0 > tY1) { float tmp = tY0; tY0 = tY1; tY1 = tmp; }
    }

    // --- Cylinder (XZ) intersection around worldOrigin ---
    const float radius = WE_CloudDomainRadius();
    const float2 ro = rayOrigin.xz - worldOrigin.xz;
    const float2 rd = rayDir.xz;
    const float a = dot(rd, rd);
    float tC0 = 0.0;
    float tC1 = cloudMaxMarchDistance;
    if (a > 1e-8)
    {
        const float b = 2.0 * dot(ro, rd);
        const float c = dot(ro, ro) - radius * radius;
        const float disc = b * b - 4.0 * a * c;
        if (disc < 0.0)
            return false;
        const float s = sqrt(disc);
        tC0 = (-b - s) / (2.0 * a);
        tC1 = (-b + s) / (2.0 * a);
        if (tC0 > tC1) { float tmp = tC0; tC0 = tC1; tC1 = tmp; }
    }
    else
    {
        // Vertical ray in XZ: must already be inside cylinder.
        if (dot(ro, ro) > radius * radius)
            return false;
    }

    // Boolean intersection of slab and cylinder intervals.
    tEnter = max(max(tY0, tC0), 0.0);
    tExit = min(min(tY1, tC1), tEnter + max(cloudMaxMarchDistance, 1000.0));
    return tExit > tEnter + 1e-2;
}

// Backward-compatible alias used by temporal/debug code.
bool WE_IntersectCloudSlab(
    float3 rayOrigin,
    float3 rayDir,
    float3 worldOrigin,
    out float t0,
    out float t1)
{
    return WE_IntersectCloudVolume(rayOrigin, rayDir, worldOrigin, t0, t1);
}

float WE_CloudLightMarch(float3 pos, float3 sunDir, float3 worldOrigin)
{
    float shadow = 1.0;
    const float top = worldOrigin.y + max(cloudTopAltitude, cloudBottomAltitude + 50.0);
    const float distToTop = max(top - pos.y, 10.0);
    const float stepLen = distToTop / 5.0;
    [unroll]
    for (int i = 0; i < 5; ++i)
    {
        const float3 samplePos = pos + sunDir * (float(i) + 0.5) * stepLen;
        const float d = WE_CloudDensityAt(samplePos, worldOrigin);
        shadow *= exp(-d * max(cloudExtinction, 0.05) * stepLen * 0.04 * max(cloudShadowStrength, 0.0));
    }
    return shadow;
}

float3 WE_RaymarchClouds(
    float3 rayOrigin,
    float3 rayDir,
    float3 worldOrigin,
    float3 sunDir,
    float3 sunColor,
    float sunIntensity,
    float2 screenUV,
    float sceneHitT, // world-space distance to opaque geometry (huge if sky)
    out float opacity,
    out float avgDensity,
    out float stepsTaken,
    out float emptySkips,
    out float tEnterOut,
    out float tExitOut)
{
    opacity = 0.0;
    avgDensity = 0.0;
    stepsTaken = 0.0;
    emptySkips = 0.0;
    tEnterOut = 0.0;
    tExitOut = 0.0;
    rayDir = normalize(rayDir);
    sunDir = normalize(sunDir);

    float t0, t1;
    if (!WE_IntersectCloudVolume(rayOrigin, rayDir, worldOrigin, t0, t1))
        return float3(0.0, 0.0, 0.0);

    // Opaque scene in front of the cloud volume → fully occluded.
    if (sceneHitT <= t0)
        return float3(0.0, 0.0, 0.0);

    // Truncate march at scene depth so clouds never draw through nearby geometry.
    t1 = min(t1, sceneHitT);
    if (t1 <= t0)
        return float3(0.0, 0.0, 0.0);

    tEnterOut = t0;
    tExitOut = t1;

    // Only rays that meaningfully look through the layer contribute (kills ground/horizon haze).
    const float elev = rayDir.y;
    // Below horizon looking down: allow only if currently inside the volume (t0\approx0).
    if (elev < -0.02 && t0 > 1.0)
        return float3(0.0, 0.0, 0.0);

    const int maxSteps = clamp(cloudQualitySteps, 12, 64);
    const float marchLen = max(t1 - t0, 1.0);

    const float viewFactor = saturate(abs(elev) * 4.0);
    const float distFactor = saturate(2500.0 / max(t0, 1.0));
    const int adaptiveSteps = clamp(
        (int)round(float(maxSteps) * lerp(0.55, 1.0, viewFactor) * lerp(0.7, 1.1, distFactor)),
        10,
        maxSteps);

    float stepSize = min(marchLen / float(adaptiveSteps), max(cloudThickness, 80.0) / 14.0);

    const float jitter = WE_BlueNoise(
        screenUV * 97.0 + float2(cloudSeed, float(cloudFrameCounter) * 0.17)
        + cloudTemporalJitter * 40.0) * stepSize;

    float3 accum = float3(0.0, 0.0, 0.0);
    float transmittance = 1.0;
    float densitySum = 0.0;
    int densityHits = 0;
    float t = t0 + jitter;

    const float3 sunLight = sunColor * sunIntensity * max(cloudLightingIntensity, 0.0);
    const float3 ambient = sunLight * max(cloudAmbient, 0.0) + float3(0.04, 0.06, 0.12);
    const float3 albedo = WE_sRGBToLinear(saturate(cloudColor));
    const float opticalScale = 0.032;

    int coarseStreak = 0;

    [loop]
    for (int i = 0; i < 64; ++i)
    {
        if (i >= adaptiveSteps || t > t1 || transmittance < 0.02)
            break;

        stepsTaken += 1.0;
        const float3 pos = rayOrigin + rayDir * t;

        const float occupancy = WE_CloudOccupancyHint(pos, worldOrigin);
        if (occupancy < cloudEmptySkipThreshold)
        {
            emptySkips += 1.0;
            coarseStreak += 1;
            t += stepSize * lerp(1.5, 3.0, saturate(float(coarseStreak) * 0.25));
            continue;
        }
        coarseStreak = 0;

        const float density = WE_CloudDensityAt(pos, worldOrigin);
        if (density <= 0.001)
        {
            t += stepSize;
            continue;
        }

        densitySum += density;
        densityHits += 1;

        const float cosTheta = dot(-rayDir, sunDir);
        const float phase = lerp(0.22, WE_HenyeyGreenstein(cosTheta, cloudPhaseG), 0.85)
            * (1.0 + max(cloudSilverLining, 0.0) * pow(saturate(cosTheta), 10.0));

        const float lightTrans = WE_CloudLightMarch(pos, sunDir, worldOrigin);
        const float powder = 1.0 - exp(-density * 2.2) * saturate(cloudPowder);
        const float multi = 1.0 + cloudMultiScatter * density * (0.5 + 0.5 * lightTrans);

        const float3 scatter =
            albedo * (ambient * (0.65 + 0.35 * lightTrans) + sunLight * phase * powder * multi * lightTrans) * density;
        const float absorb = exp(-density * max(cloudExtinction, 0.05) * stepSize * opticalScale);
        accum += transmittance * scatter * (1.0 - absorb);
        transmittance *= absorb;

        t += stepSize * lerp(0.55, 1.0, saturate(1.0 - density));
    }

    avgDensity = densityHits > 0 ? (densitySum / float(densityHits)) : 0.0;
    const float rawOpacity = saturate(1.0 - transmittance);
    // Distance fade: far hits within the domain stay softer so they read as a layer, not a veil.
    const float distFade = 1.0 - saturate((t0 - 500.0) / max(WE_CloudDomainRadius(), 1.0));
    opacity = rawOpacity * lerp(0.55, 1.0, saturate(elev * 3.0 + 0.15)) * lerp(0.75, 1.0, distFade);
    if (rawOpacity <= 1e-4)
        return float3(0.0, 0.0, 0.0);

    return (accum / max(rawOpacity, 1e-3)) * 1.1;
}

#endif // WE_VOLUMETRIC_CLOUDS_HLSLI
