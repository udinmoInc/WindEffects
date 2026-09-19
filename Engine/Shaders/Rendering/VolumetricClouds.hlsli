#ifndef WE_VOLUMETRIC_CLOUDS_HLSLI
#define WE_VOLUMETRIC_CLOUDS_HLSLI

// Horizon Zero Dawn / Nubis volumetric cloud helpers (Schneider, SIGGRAPH 2015 / GPU Pro 7).
#include "../Common/Math.hlsli"

static const float WE_CLOUD_PLANET_RADIUS_M = 6371000.0;
static const float WE_CLOUD_INNER_OFFSET_M  = 1500.0;
static const float WE_CLOUD_OUTER_OFFSET_M  = 4000.0;

// Horizon path / aerial perspective (meters).
static const float WE_CLOUD_MAX_MARCH_M           = 35000.0;
static const float WE_CLOUD_HORIZON_MAX_MARCH_M   = 22000.0;
static const float WE_CLOUD_HORIZON_FADE_START_M  = 12000.0;
static const float WE_CLOUD_HORIZON_FADE_END_M    = 35000.0;
static const float WE_CLOUD_ATMO_EXTINCTION       = 0.000028;

float Remap(float value, float originalMin, float originalMax, float newMin, float newMax)
{
    return newMin + ((value - originalMin) / max(originalMax - originalMin, 1e-5)) * (newMax - newMin);
}

float RemapClamped(float value, float originalMin, float originalMax, float newMin, float newMax)
{
    return saturate(Remap(value, originalMin, originalMax, newMin, newMax));
}

bool RaySphereIntersect(
    float3 rayOrigin,
    float3 rayDir,
    float sphereRadius,
    out float t0,
    out float t1)
{
    t0 = 0.0;
    t1 = 0.0;
    rayDir = normalize(rayDir);
    const float b = dot(rayOrigin, rayDir);
    const float c = dot(rayOrigin, rayOrigin) - sphereRadius * sphereRadius;
    const float discriminant = b * b - c;
    if (discriminant < 0.0)
        return false;
    const float s = sqrt(max(discriminant, 0.0));
    t0 = -b - s;
    t1 = -b + s;
    return t1 > 0.0;
}

// First positive hit distance, or a huge value if none.
float RaySphereHitPositive(float3 rayOrigin, float3 rayDir, float sphereRadius)
{
    float t0, t1;
    if (!RaySphereIntersect(rayOrigin, rayDir, sphereRadius, t0, t1))
        return 1e9;
    if (t0 > 1e-3)
        return t0;
    if (t1 > 1e-3)
        return t1;
    return 1e9;
}

float3 WorldToPlanetSpace(float3 worldPos, float3 worldOrigin, float planetRadiusMeters)
{
    // Planet center at worldOrigin - (0,R,0); planet-space = offset + (0,R,0).
    return (worldPos - worldOrigin) + float3(0.0, planetRadiusMeters, 0.0);
}

float CloudHeightFraction(float3 planetPos, float innerRadius, float outerRadius)
{
    // World-Y band (matches surface slab + flat landscape). Radial length()
    // tilted the deck when the camera was offset in XZ.
    return saturate((planetPos.y - innerRadius) / max(outerRadius - innerRadius, 1.0));
}

float3 ApproximateSkyRadiance(float3 rayDir, float3 toSun, float3 sunCol, float3 ambientCol)
{
    const float height = saturate(rayDir.y * 0.5 + 0.5);
    const float3 horizon = float3(0.55, 0.62, 0.72);
    const float3 zenith  = float3(0.12, 0.28, 0.58);
    const float3 nadir   = float3(0.22, 0.24, 0.26);
    float3 sky = lerp(horizon, zenith, pow(height, 1.25));
    sky = lerp(nadir, sky, saturate(rayDir.y * 0.85 + 0.85));
    sky += sunCol * pow(saturate(dot(rayDir, toSun)), 8.0) * 0.35;
    sky += ambientCol * 0.12;
    return sky;
}

// ---------------------------------------------------------------------------
// Cloud shell interval — surface cameras use a WORLD-Y altitude slab so the
// cloud deck stays parallel to the flat landscape (not camera / planet-radial).
// ---------------------------------------------------------------------------
bool CloudShellIntersect(
    float3 planetCam,
    float3 rayDir,
    float planetRadius,
    float innerRadius,
    float outerRadius,
    out float tStart,
    out float tEnd)
{
    tStart = 0.0;
    tEnd = 0.0;
    rayDir = normalize(rayDir);

    const float rCam = length(planetCam);
    // World up is +Y (engine / landscape). Do NOT use normalize(planetCam) here —
    // that tilts the slab when the camera is offset from origin and makes the
    // cloud deck appear to bank with the view relative to the ground plane.
    const float3 worldUp = float3(0.0, 1.0, 0.0);
    const float camHeight = planetCam.y - planetRadius; // meters AGL (Y-up)
    const float cloudBottom = innerRadius - planetRadius;
    const float cloudTop = outerRadius - planetRadius;
    const float elev = rayDir.y; // == dot(rayDir, worldUp)

    // ---- Surface / below cloud bottom ----
    if (rCam < innerRadius || camHeight < cloudBottom)
    {
        // Looking down / into the ground — no cloud march.
        if (elev < 0.02)
            return false;

        tStart = (cloudBottom - camHeight) / elev;
        tEnd = (cloudTop - camHeight) / elev;
        if (tEnd < tStart)
        {
            float tmp = tStart;
            tStart = tEnd;
            tEnd = tmp;
        }

        if (camHeight > cloudBottom && camHeight < cloudTop)
            tStart = 0.0;

        tStart = max(tStart, 0.0);
        if (tEnd <= tStart + 1.0)
            return false;

        const float horizon = saturate(1.0 - elev * 5.0);
        const float maxPath = lerp(WE_CLOUD_MAX_MARCH_M, WE_CLOUD_HORIZON_MAX_MARCH_M, horizon);
        tEnd = min(tEnd, tStart + maxPath);
        return tEnd > tStart + 1.0;
    }

    // ---- Inside / above shell: planetary spheres ----
    float tOuter0 = 0.0;
    float tOuter1 = 0.0;
    if (!RaySphereIntersect(planetCam, rayDir, outerRadius, tOuter0, tOuter1))
        return false;

    float tInner0 = 0.0;
    float tInner1 = 0.0;
    const bool hitInner = RaySphereIntersect(planetCam, rayDir, innerRadius, tInner0, tInner1);
    const float tGround = RaySphereHitPositive(planetCam, rayDir, planetRadius);

    if (rCam < outerRadius)
    {
        tStart = 0.0;
        tEnd = tOuter1;
        if (hitInner && tInner0 > 0.0)
            tEnd = min(tEnd, tInner0);
        tEnd = min(tEnd, tGround);
    }
    else
    {
        tStart = max(tOuter0, 0.0);
        tEnd = tOuter1;
        if (hitInner && tInner0 > tStart)
            tEnd = tInner0;
        tEnd = min(tEnd, tGround);
    }

    if (tEnd <= tStart + 1.0)
        return false;

    const float horizon = saturate(1.0 - max(elev, 0.0) * 5.0);
    const float maxPath = lerp(WE_CLOUD_MAX_MARCH_M, WE_CLOUD_HORIZON_MAX_MARCH_M, horizon);
    tEnd = min(tEnd, tStart + maxPath);
    return tEnd > tStart + 1.0;
}

// Height fraction from world-Y altitude (matches the surface slab).
float CloudHeightFractionY(float3 planetPos, float planetRadius, float innerRadius, float outerRadius)
{
    const float altitude = planetPos.y - planetRadius;
    const float bottom = innerRadius - planetRadius;
    const float top = outerRadius - planetRadius;
    return saturate((altitude - bottom) / max(top - bottom, 1.0));
}

// ---------------------------------------------------------------------------
float StratusGradient(float h)
{
    return saturate(Remap(h, 0.0, 0.1, 0.0, 1.0)) * saturate(Remap(h, 0.2, 0.3, 1.0, 0.0));
}

float CumulusGradient(float h)
{
    // Flat base, strong mid puff, soft top — used on *local* height (per-pack).
    return saturate(Remap(h, 0.0, 0.18, 0.0, 1.0)) * saturate(Remap(h, 0.55, 1.0, 1.0, 0.0));
}

float CumulonimbusGradient(float h)
{
    return saturate(Remap(h, 0.0, 0.10, 0.0, 1.0)) * saturate(Remap(h, 0.80, 1.0, 1.0, 0.0));
}

float CloudTypeHeightSignal(float heightFraction, float cloudType)
{
    const float stratus = StratusGradient(heightFraction);
    const float cumulus = CumulusGradient(heightFraction);
    const float cumulonimbus = CumulonimbusGradient(heightFraction);
    const float t0 = saturate(cloudType * 2.0);
    const float t1 = saturate(cloudType * 2.0 - 1.0);
    return lerp(lerp(stratus, cumulus, t0), cumulonimbus, t1);
}

// Break axis-aligned tile lines that read as a horizon grid.
float2 CloudShearXZ(float2 xz)
{
    return float2(
        xz.x * 0.965 + xz.y * 0.255,
        xz.x * -0.215 + xz.y * 1.035);
}

float HenyeyGreenstein(float cosTheta, float g)
{
    const float g2 = g * g;
    const float denom = pow(max(1.0 + g2 - 2.0 * g * cosTheta, 1e-4), 1.5);
    return (1.0 - g2) / max(4.0 * WE_PI * denom, 1e-4);
}

float DualLobeHG(float cosTheta, float gForward, float gBack, float blend)
{
    return lerp(HenyeyGreenstein(cosTheta, gBack), HenyeyGreenstein(cosTheta, gForward), blend);
}

float BeerLaw(float densityAlongLight, float absorption)
{
    return exp(-densityAlongLight * absorption);
}

// Schneider / Nubis powder: 1 - exp(-od * 2). Keep most of the classic term.
float PowderEffect(float densityAlongLight, float cosTheta, float powderStrength)
{
    const float powder = 1.0 - exp(-densityAlongLight * 2.0);
    const float viewDependent = saturate((-cosTheta) * 0.5 + 0.5);
    const float apply = saturate(powderStrength) * lerp(0.35, 0.85, viewDependent);
    return lerp(1.0, powder, apply);
}

float SilverLining(float cosTheta, float intensity, float spread)
{
    const float lobe = pow(saturate(cosTheta), max(spread, 1.0));
    return 1.0 + saturate(intensity) * lobe * 0.55;
}

// Andrew Schneider (HZD / GPU Pro 7):
//   beer   = exp(-od * sigma)           sigma ~ 0.05..0.1
//   powder = 1 - exp(-od * 2)
//   direct = 2 * beer * powder
// Multi-scatter fills thick cores. Beer floor keeps sunlit faces from going ambient-blue.
float BeerPowderMultiScatter(float optical, float absorption, float cosTheta, float powderStrength)
{
    const float sigma = clamp(absorption, 0.045, 0.12);
    const float beer = BeerLaw(optical, sigma);
    const float powder = PowderEffect(optical, cosTheta, powderStrength);
    // Classic term — but at OD~0 powder→0 would kill all direct (blue ambient blobs).
    const float beerPowder = 2.0 * beer * powder;
    const float sunlitFloor = beer * 0.45;
    const float direct = max(beerPowder, sunlitFloor);

    const float ms1 = BeerLaw(optical, sigma * 0.50);
    const float ms2 = BeerLaw(optical, sigma * 0.25);
    const float ms3 = BeerLaw(optical, sigma * 0.125);
    const float multiScatter = beer * 0.50 + ms1 * 0.28 + ms2 * 0.15 + ms3 * 0.07;

    return direct * 0.75 + multiScatter * (1.0 - beer) * 0.70;
}

float HeightAmbientFactor(float heightFraction)
{
    // Brighter cauliflower tops — sky-like, not mud (single definition).
    return lerp(0.55, 1.35, saturate(heightFraction));
}

// ---------------------------------------------------------------------------
struct CloudDensityResult
{
    float density;
    float heightFraction;
};

CloudDensityResult SampleCloudDensity(
    float3 planetPos,
    float innerRadius,
    float outerRadius,
    float coverage,
    float cloudType,
    float precipitation,
    float3 windOffset,
    float baseScale,
    float detailScale,
    float curlStrength,
    bool expensive,
    Texture3D<float4> baseShape,
    Texture3D<float4> detailShape,
    Texture2D<float4> curlNoise,
    SamplerState samp)
{
    CloudDensityResult result;
    result.density = 0.0;
    result.heightFraction = CloudHeightFraction(planetPos, innerRadius, outerRadius);

    if (result.heightFraction <= 0.0 || result.heightFraction >= 1.0)
        return result;

    // World meters: XZ from planet frame, Y = altitude above cloud bottom.
    const float3 worldPos = float3(
        planetPos.x,
        planetPos.y - innerRadius,
        planetPos.z);

    const float bScale = max(baseScale, 0.00012);
    const float dScale = max(detailScale, 0.0012);

    // --- Anti-grid weather: shear + domain warp + irrational octaves ---
    // Axis-aligned frac(XZ * scale) tiles read as horizon rows/columns.
    float2 xz = worldPos.xz + windOffset.xz * 0.4;
    xz = CloudShearXZ(xz);

    // Large-scale curl warp so packs drift off the lattice.
    {
        const float2 warpUV = frac(xz * 0.00007 + float2(0.13, 0.41));
        const float2 warp = (curlNoise.SampleLevel(samp, warpUV, 0.0).rg * 2.0 - 1.0);
        xz += warp * (2200.0 * max(curlStrength, 0.25));
    }

    const float weatherScale = bScale * 0.42;
    // Irrational frequency ratios break repeating cell lines.
    const float3 wA = float3(xz * weatherScale, 0.17);
    const float3 wB = float3(xz * (weatherScale * 1.618) + float2(17.3, 9.1), 0.41);
    const float3 wC = float3(CloudShearXZ(xz.yx) * (weatherScale * 0.618) + float2(3.7, 22.5), 0.63);

    const float4 s0 = baseShape.SampleLevel(samp, frac(wA), 0.0);
    const float4 s1 = baseShape.SampleLevel(samp, frac(wB), 0.0);
    const float4 s2 = baseShape.SampleLevel(samp, frac(wC), 0.0);

    // Blend Worley islands with a little Perlin so clusters clump organically.
    float weather = saturate(
        s0.g * 0.40 + s0.b * 0.20 +
        s1.g * 0.22 +
        s2.r * 0.18);
    weather = RemapClamped(weather, 0.28, 0.90, 0.0, 1.0);
    weather = pow(weather, 1.18);

    const float coveragePrime = saturate(coverage);
    float weatherCloud = RemapClamped(weather, 1.0 - coveragePrime, 1.0, 0.0, 1.0);
    weatherCloud = pow(saturate(weatherCloud * max(weather, 0.35)), 1.05);
    if (weatherCloud < 0.02)
        return result;

    // --- Per-pack elevation: each weather cell gets its own altitude band ---
    // (Fixes the flat “all clouds on one shelf” look.)
    const float3 altPos = float3(xz * (weatherScale * 0.31) + float2(5.2, 11.8), 0.07);
    const float altNoise = baseShape.SampleLevel(samp, frac(altPos), 0.0).r;
    const float packHeight = saturate(s1.a * 0.55 + altNoise * 0.45);
    // Local slab window inside the global cloud layer.
    const float localBottom = lerp(0.02, 0.38, packHeight);
    const float localThickness = lerp(0.40, 0.72, saturate(weather * 0.6 + packHeight * 0.4));
    const float localTop = min(localBottom + localThickness, 0.98);
    const float localH = RemapClamped(result.heightFraction, localBottom, localTop, 0.0, 1.0);
    if (localH <= 0.0 || localH >= 1.0)
        return result;

    const float heightSignal = CloudTypeHeightSignal(localH, cloudType);
    if (heightSignal <= 1e-4)
        return result;

    // --- Base shape (mild Y stretch for billows, still permissive) ---
    float3 basePos = float3(xz, worldPos.y) * bScale;
    basePos.y *= lerp(1.05, 1.35, packHeight);

    if (curlStrength > 1e-4)
    {
        const float2 curlUV = frac(xz * 0.00028 + windOffset.xz * 0.1);
        const float2 curl = (curlNoise.SampleLevel(samp, curlUV, 0.0).rg * 2.0 - 1.0) * curlStrength;
        basePos.xz += curl * 0.40;
        basePos.y += (curl.x * 0.08 + curl.y * 0.06);
    }

    const float4 lowFreq = baseShape.SampleLevel(samp, frac(basePos), 0.0);
    const float lowFreqFBM = lowFreq.g * 0.625 + lowFreq.b * 0.25 + lowFreq.a * 0.125;
    float baseCloud = saturate(Remap(lowFreq.r, lowFreqFBM - 1.0, 1.0, 0.0, 1.0));

    baseCloud *= heightSignal;
    baseCloud = RemapClamped(baseCloud, 1.0 - weatherCloud, 1.0, 0.0, 1.0);
    baseCloud *= weatherCloud;
    // Soft local underside (use localH, not global slab floor).
    baseCloud *= RemapClamped(localH, 0.0, 0.18, 0.0, 1.0);

    if (baseCloud < 0.015)
        return result;

    // --- Detail erosion ---
    {
        float3 detailPos = float3(xz, worldPos.y) * dScale + windOffset * 0.5 * dScale;
        if (expensive && curlStrength > 1e-4)
        {
            const float2 curlUV = frac(detailPos.xz * 0.5);
            const float2 curl = (curlNoise.SampleLevel(samp, curlUV, 0.0).rg * 2.0 - 1.0) * curlStrength;
            detailPos.xz += curl * 0.45;
        }

        const float4 highFreq = detailShape.SampleLevel(samp, frac(detailPos), 0.0);
        float highFreqFBM = highFreq.r * 0.625 + highFreq.g * 0.25 + highFreq.b * 0.125;
        highFreqFBM = lerp(1.0 - highFreqFBM, highFreqFBM, saturate(localH * 5.0));

        const float erodeStr = expensive ? 0.48 : 0.32;
        baseCloud = RemapClamped(baseCloud, highFreqFBM * erodeStr, 1.0, 0.0, 1.0);
    }

    if (baseCloud < 0.015)
        return result;

    const float rainDarken = lerp(1.0, 0.65, saturate(precipitation));
    result.density = max(baseCloud * rainDarken, 0.0);
    return result;
}

#endif // WE_VOLUMETRIC_CLOUDS_HLSLI
