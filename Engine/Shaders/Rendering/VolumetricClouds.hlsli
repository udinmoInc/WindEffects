#ifndef WE_VOLUMETRIC_CLOUDS_HLSLI
#define WE_VOLUMETRIC_CLOUDS_HLSLI

#include "../Common/Noise.hlsli"
#include "../Common/Color.hlsli"

static const int WE_CLOUD_STEPS = 24;

float WE_CloudDensityAt(float3 worldPos, float3 worldOrigin, float cloudAltitude, float coverage)
{
    const float3 rel = worldPos - worldOrigin;
    const float heightKm = rel.y * 0.001;
    const float layerCenter = cloudAltitude * 0.001;
    const float layerHalf = 0.4;

    const float heightMask = 1.0 - saturate(abs(heightKm - layerCenter) / layerHalf);
    if (heightMask <= 0.0)
        return 0.0;

    const float3 noisePos = rel * 0.00012 + float3(0.0, layerCenter * 10.0, 0.0);
    const float base = WE_FBM3D(noisePos, 5);
    const float detail = WE_FBM3D(noisePos * 3.7 + 17.3, 3);
    const float density = saturate((base * 0.75 + detail * 0.25 - (1.0 - coverage)) * 2.2);
    return density * heightMask;
}

float3 WE_RaymarchClouds(
    float3 rayOrigin,
    float3 rayDir,
    float3 worldOrigin,
    float3 sunDir,
    float3 sunColor,
    float sunIntensity,
    float cloudAltitude,
    float coverage,
    float extinction,
    float3 cloudAlbedo)
{
    rayDir = normalize(rayDir);
    sunDir = normalize(sunDir);

    const float layerY = worldOrigin.y + cloudAltitude;
    const float tHit = (layerY - rayOrigin.y) / max(rayDir.y, 1e-4);
    if (tHit < 0.0)
        return float3(0.0, 0.0, 0.0);

    const float stepSize = 120.0;
    float3 accum = float3(0.0, 0.0, 0.0);
    float transmittance = 1.0;
    const float3 sunLight = sunColor * sunIntensity;
    // Hemisphere skylight fill so cloud undersides stay visible after tonemap.
    const float3 ambient = sunLight * 0.18 + float3(0.015, 0.02, 0.035);

    [loop]
    for (int i = 0; i < WE_CLOUD_STEPS; ++i)
    {
        const float t = tHit + float(i) * stepSize;
        const float3 pos = rayOrigin + rayDir * t;
        const float density = WE_CloudDensityAt(pos, worldOrigin, cloudAltitude, coverage);
        if (density <= 0.001)
            continue;

        const float cosTheta = saturate(dot(-rayDir, sunDir));
        const float phase = 0.2 + 0.8 * pow(cosTheta, 6.0);
        const float3 scatter = cloudAlbedo * (ambient + sunLight * phase * 0.85) * density;
        const float absorb = exp(-density * extinction * stepSize * 0.008);
        accum += transmittance * scatter * (1.0 - absorb);
        transmittance *= absorb;
        if (transmittance < 0.02)
            break;
    }

    return accum * 1.6;
}

#endif // WE_VOLUMETRIC_CLOUDS_HLSLI
