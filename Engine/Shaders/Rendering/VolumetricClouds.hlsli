#ifndef WE_VOLUMETRIC_CLOUDS_HLSLI
#define WE_VOLUMETRIC_CLOUDS_HLSLI

#include "../Common/Noise.hlsli"
#include "../Common/Color.hlsli"
#include "../Common/EnvironmentBuffer.hlsli"

float WE_CloudDensityAt(float3 worldPos, float3 worldOrigin)
{
    const float3 rel = worldPos - worldOrigin;
    const float heightM = rel.y;
    const float bottom = cloudBottomAltitude;
    const float top = max(cloudTopAltitude, bottom + 50.0);
    const float heightMask = saturate((heightM - bottom) / max(top - bottom, 1.0));
    const float heightProfile = saturate(1.0 - abs(heightMask * 2.0 - 1.0));
    if (heightProfile <= 0.0)
        return 0.0;

    const float3 wind = normalize(cloudWindDir + float3(1e-4, 0.0, 0.0)) * (cloudWindSpeed * cloudAnimTime);
    const float3 noisePos = (rel + wind) * (0.00012 * max(cloudNoiseScale, 0.05))
        + float3(cloudSeed, cloudAltitude * 0.001 * 10.0, cloudSeed * 0.37);

    const float base = WE_FBM3D(noisePos, 5);
    const float detail = WE_FBM3D(noisePos * max(cloudDetailScale, 1.0) + 17.3, 3);
    const float shaped = base * saturate(cloudShapeNoise) + detail * saturate(cloudErosionNoise);
    const float coverageGate = saturate((shaped - (1.0 - cloudCoverage)) * 2.2);
    return coverageGate * heightProfile * max(cloudDensityMult, 0.0);
}

float WE_HenyeyGreenstein(float cosTheta, float g)
{
    const float g2 = g * g;
    const float denom = pow(max(1.0 + g2 - 2.0 * g * cosTheta, 1e-4), 1.5);
    return (1.0 - g2) / (4.0 * 3.14159265 * denom);
}

float3 WE_RaymarchClouds(
    float3 rayOrigin,
    float3 rayDir,
    float3 worldOrigin,
    float3 sunDir,
    float3 sunColor,
    float sunIntensity,
    out float opacity)
{
    opacity = 0.0;
    rayDir = normalize(rayDir);
    sunDir = normalize(sunDir);

    // Intersect vertical cloud slab.
    const float bottom = worldOrigin.y + cloudBottomAltitude;
    const float top = worldOrigin.y + cloudTopAltitude;
    float t0 = (bottom - rayOrigin.y) / (rayDir.y == 0.0 ? 1e-4 : rayDir.y);
    float t1 = (top - rayOrigin.y) / (rayDir.y == 0.0 ? 1e-4 : rayDir.y);
    if (t0 > t1) { float tmp = t0; t0 = t1; t1 = tmp; }
    t0 = max(t0, 0.0);
    if (t1 < 0.0)
        return float3(0.0, 0.0, 0.0);

    const float horizonFade = smoothstep(0.015, 0.1, abs(rayDir.y));
    if (horizonFade <= 1e-4)
        return float3(0.0, 0.0, 0.0);

    const int steps = clamp(cloudQualitySteps, 8, 64);
    const float marchLen = max(t1 - t0, 1.0);
    const float stepSize = marchLen / float(steps);
    const float jitter = WE_BlueNoise(rayDir.xz * 37.0 + float2(cloudSeed, 0.61) + cloudAnimTime * 0.01) * stepSize;

    float3 accum = float3(0.0, 0.0, 0.0);
    float transmittance = 1.0;
    const float3 sunLight = sunColor * sunIntensity * max(cloudLightingIntensity, 0.0);
    const float3 ambient = sunLight * max(cloudAmbient, 0.0) + float3(0.05, 0.08, 0.14);
    const float3 albedo = WE_sRGBToLinear(saturate(cloudColor));

    [loop]
    for (int i = 0; i < 64; ++i)
    {
        if (i >= steps)
            break;

        const float t = t0 + jitter + float(i) * stepSize;
        if (t > t1)
            break;

        const float3 pos = rayOrigin + rayDir * t;
        const float density = WE_CloudDensityAt(pos, worldOrigin);
        if (density <= 0.001)
            continue;

        const float cosTheta = dot(-rayDir, sunDir);
        const float phase = lerp(0.2, WE_HenyeyGreenstein(cosTheta, cloudPhaseG), 0.85)
            * (1.0 + max(cloudSilverLining, 0.0) * pow(saturate(cosTheta), 8.0));

        // Powder sugar approximation for dark edges when lit from behind.
        const float powder = 1.0 - exp(-density * 2.0) * saturate(cloudPowder);
        const float multi = 1.0 + cloudMultiScatter * density;

        const float3 scatter = albedo * (ambient + sunLight * phase * powder * multi) * density;
        const float absorb = exp(-density * cloudExtinction * stepSize * 0.008);
        accum += transmittance * scatter * (1.0 - absorb);
        transmittance *= absorb;
        if (transmittance < 0.02)
            break;
    }

    const float rawOpacity = saturate(1.0 - transmittance);
    opacity = rawOpacity * horizonFade;
    if (rawOpacity <= 1e-4)
        return float3(0.0, 0.0, 0.0);

    return (accum / rawOpacity) * 1.1;
}

#endif // WE_VOLUMETRIC_CLOUDS_HLSLI
