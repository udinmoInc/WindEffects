#ifndef WE_SKY_ATMOSPHERE_HLSLI
#define WE_SKY_ATMOSPHERE_HLSLI

#include "../Common/Math.hlsli"
#include "../Common/Color.hlsli"

// Physically based procedural sky / atmosphere (no textures, meshes, or cubemaps).
// Modular hooks for future clouds, moon, stars, and weather extensions.

static const float WE_PLANET_RADIUS_KM       = 6360.0;
static const float WE_ATMOSPHERE_HEIGHT_KM   = 60.0;
static const float WE_RAYLEIGH_SCALE_KM      = 8.0;
static const float WE_MIE_SCALE_KM           = 1.2;
static const float WE_SUN_ANGULAR_RADIUS     = 0.004675;
static const int   WE_ATMOSPHERE_STEPS       = 16;
static const int   WE_SUN_TRANSMITTANCE_STEPS = 8;

struct WE_AtmosphereParams
{
    float3 rayleighCoeff;
    float  mieCoeff;
    float  mieAnisotropy;
    float  planetRadius;
    float  atmosphereRadius;
    float3 sunColor;
    float  sunIntensity;
    float3 fogColor;
    float  fogDensity;
};

float WE_RayleighPhase(float cosTheta)
{
    return (3.0 / (16.0 * WE_PI)) * (1.0 + cosTheta * cosTheta);
}

float WE_MiePhase(float cosTheta, float g)
{
    const float g2 = g * g;
    const float num = (1.0 - g2);
    const float denom = pow(max(1.0 + g2 - 2.0 * g * cosTheta, 1e-4), 1.5);
    return (3.0 / (8.0 * WE_PI)) * ((1.0 + g2) * num / denom);
}

float WE_AtmosphereDensity(float height, float scaleHeight)
{
    return exp(-max(height, 0.0) / max(scaleHeight, 1e-3));
}

bool WE_IntersectSphere(float3 origin, float3 dir, float radius, out float t0, out float t1)
{
    const float b = dot(origin, dir);
    const float c = dot(origin, origin) - radius * radius;
    const float discriminant = b * b - c;
    if (discriminant < 0.0)
        return false;

    const float s = sqrt(discriminant);
    t0 = -b - s;
    t1 = -b + s;
    return t1 > 0.0;
}

float3 WE_IntegrateSunTransmittance(float3 origin, float3 sunDir, WE_AtmosphereParams params)
{
    const float mu = dot(normalize(origin), sunDir);
    const float rayleighOpticalDepth = 0.0;
    float mieOpticalDepth = 0.0;

    float3 opticalDepthRayleigh = float3(0.0, 0.0, 0.0);
    float3 opticalDepthMie = float3(0.0, 0.0, 0.0);

    const float stepSize = (params.atmosphereRadius - params.planetRadius) / float(WE_SUN_TRANSMITTANCE_STEPS);
    float3 samplePoint = origin;

    [unroll]
    for (int i = 0; i < WE_SUN_TRANSMITTANCE_STEPS; ++i)
    {
        const float height = length(samplePoint) - params.planetRadius;
        const float rayleighDensity = WE_AtmosphereDensity(height, WE_RAYLEIGH_SCALE_KM);
        const float mieDensity = WE_AtmosphereDensity(height, WE_MIE_SCALE_KM);

        opticalDepthRayleigh += params.rayleighCoeff * rayleighDensity * stepSize;
        opticalDepthMie += params.mieCoeff * mieDensity * stepSize;

        samplePoint += sunDir * stepSize;
    }

    (void)mu;
    (void)rayleighOpticalDepth;
    (void)mieOpticalDepth;

    const float3 tau = opticalDepthRayleigh + opticalDepthMie;
    return exp(-tau);
}

float3 WE_ComputeAtmosphereScattering(
    float3 viewDir,
    float3 sunDir,
    WE_AtmosphereParams params)
{
    viewDir = normalize(viewDir);
    sunDir = normalize(sunDir);

    const float3 origin = float3(0.0, params.planetRadius + 0.001, 0.0);
    float t0 = 0.0;
    float t1 = 0.0;
    if (!WE_IntersectSphere(origin, viewDir, params.atmosphereRadius, t0, t1))
        return float3(0.0, 0.0, 0.0);

    const float start = max(t0, 0.0);
    const float end = t1;
    const float stepSize = (end - start) / float(WE_ATMOSPHERE_STEPS);

    float3 sumRayleigh = float3(0.0, 0.0, 0.0);
    float3 sumMie = float3(0.0, 0.0, 0.0);
    float3 opticalDepthRayleigh = float3(0.0, 0.0, 0.0);
    float3 opticalDepthMie = float3(0.0, 0.0, 0.0);

    const float cosTheta = dot(viewDir, sunDir);
    const float phaseRayleigh = WE_RayleighPhase(cosTheta);
    const float phaseMie = WE_MiePhase(cosTheta, params.mieAnisotropy);

    [loop]
    for (int i = 0; i < WE_ATMOSPHERE_STEPS; ++i)
    {
        const float t = start + (float(i) + 0.5) * stepSize;
        const float3 samplePoint = origin + viewDir * t;
        const float height = length(samplePoint) - params.planetRadius;

        const float rayleighDensity = WE_AtmosphereDensity(height, WE_RAYLEIGH_SCALE_KM);
        const float mieDensity = WE_AtmosphereDensity(height, WE_MIE_SCALE_KM);

        opticalDepthRayleigh += params.rayleighCoeff * rayleighDensity * stepSize;
        opticalDepthMie += params.mieCoeff * mieDensity * stepSize;

        const float3 sunTransmittance = WE_IntegrateSunTransmittance(samplePoint, sunDir, params);
        const float3 tau = opticalDepthRayleigh + opticalDepthMie;
        const float3 attenuation = exp(-tau) * sunTransmittance;

        sumRayleigh += attenuation * rayleighDensity * stepSize;
        sumMie += attenuation * mieDensity * stepSize;
    }

    const float3 sunRadiance = params.sunColor * params.sunIntensity;
  return sunRadiance * (sumRayleigh * phaseRayleigh * params.rayleighCoeff + sumMie * phaseMie * params.mieCoeff);
}

float3 WE_ComputeSunDisk(float3 viewDir, float3 sunDir, float intensity, float3 sunColor)
{
    const float cosAngle = dot(normalize(viewDir), normalize(sunDir));
    const float cosRadius = cos(WE_SUN_ANGULAR_RADIUS);

    // Core disk
    const float disk = smoothstep(cosRadius, cosRadius + 0.00035, cosAngle);

    // Mie glow / corona
    const float glowAngle = max(0.0, cosAngle - cosRadius);
    const float glow = pow(saturate(1.0 - glowAngle * 120.0), 3.0) * 0.35;
    const float mieGlow = pow(saturate(dot(viewDir, sunDir)), 256.0) * intensity * 0.08;

    return sunColor * intensity * (disk * 25.0 + glow + mieGlow);
}

float3 WE_ComputeHorizonHaze(float3 viewDir, float3 sunDir, WE_AtmosphereParams params)
{
    const float horizonFactor = pow(saturate(1.0 - abs(viewDir.y)), 4.0);
    const float sunProximity = pow(saturate(dot(normalize(viewDir), normalize(sunDir))), 8.0);
    const float3 horizonTint = lerp(params.fogColor, params.sunColor, sunProximity * 0.65);
    return horizonTint * horizonFactor * params.fogDensity * 2.5;
}

float3 WE_ComputeNightSky(float3 viewDir, float sunElevation)
{
    const float nightFactor = saturate(-sunElevation * 4.0);
    const float3 nightZenith = float3(0.003, 0.005, 0.012);
    const float3 nightHorizon = float3(0.01, 0.008, 0.006);
    const float up = saturate(viewDir.y * 0.5 + 0.5);
    return lerp(nightHorizon, nightZenith, pow(up, 1.4)) * nightFactor;
}

WE_AtmosphereParams WE_BuildAtmosphereParams(
    float3 rayleighCoeff,
    float mieCoeff,
    float mieAnisotropy,
    float planetRadius,
    float atmosphereHeight,
    float3 sunColor,
    float sunIntensity,
    float3 fogColor,
    float fogDensity)
{
    WE_AtmosphereParams p;
    p.rayleighCoeff = max(rayleighCoeff, float3(1e-6, 1e-6, 1e-6));
    p.mieCoeff = max(mieCoeff, 1e-6);
    p.mieAnisotropy = mieAnisotropy;
    p.planetRadius = max(planetRadius, 1.0);
    p.atmosphereRadius = p.planetRadius + max(atmosphereHeight, 0.1);
    p.sunColor = max(sunColor, float3(0.0, 0.0, 0.0));
    p.sunIntensity = max(sunIntensity, 0.0);
    p.fogColor = fogColor;
    p.fogDensity = fogDensity;
    return p;
}

float3 WE_SampleSkyAtmosphere(
    float3 worldDir,
    float3 lightTravelDir,
    float3 sunColor,
    float sunIntensity,
    float3 rayleighCoeff,
    float mieCoeff,
    float mieAnisotropy,
    float planetRadius,
    float atmosphereHeight,
    float3 fogColor,
    float fogDensity)
{
    worldDir = normalize(worldDir);
    const float3 sunDir = normalize(-lightTravelDir);
    const float sunElevation = sunDir.y;

    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleighCoeff, mieCoeff, mieAnisotropy,
        planetRadius, atmosphereHeight,
        sunColor, sunIntensity, fogColor, fogDensity);

    float3 sky = WE_ComputeAtmosphereScattering(worldDir, sunDir, params);
    sky += WE_ComputeSunDisk(worldDir, sunDir, sunIntensity, sunColor);
    sky += WE_ComputeHorizonHaze(worldDir, sunDir, params);
    sky += WE_ComputeNightSky(worldDir, sunElevation);

    // Horizon extinction — smooth blend into aerial perspective without hard lines.
    const float horizonFade = pow(saturate(1.0 - abs(worldDir.y)), 6.0);
    const float3 extinction = exp(-params.rayleighCoeff * 800.0 * horizonFade);
    sky *= extinction;
    sky = lerp(sky, params.fogColor * 0.5, horizonFade * saturate(params.fogDensity * 3.0));

    return max(sky, 0.0);
}

// Analytic sky-light capture for hemisphere ambient (CPU mirrors this).
float3 WE_SampleSkyLightUpper(float3 lightTravelDir, float3 rayleighCoeff, float3 sunColor, float sunIntensity)
{
    const float3 sunDir = normalize(-lightTravelDir);
    const float elevation = saturate(sunDir.y);
    const float3 zenith = float3(0.18, 0.38, 0.95) * lerp(0.15, 1.0, elevation);
    const float3 warm = sunColor * sunIntensity * 0.04 * (1.0 - elevation);
    return zenith + warm;
}

float3 WE_SampleSkyLightLower(float3 lightTravelDir, float3 fogColor, float fogDensity)
{
    const float3 sunDir = normalize(-lightTravelDir);
    const float elevation = saturate(sunDir.y);
    const float night = saturate(1.0 - elevation * 2.0);
    return lerp(fogColor * 0.35, float3(0.02, 0.025, 0.04), night) * (0.5 + fogDensity);
}

float WE_ComputeExposureEV(float3 lightTravelDir, float baseEV)
{
    const float sunElevation = normalize(-lightTravelDir).y;
    const float dayFactor = saturate(sunElevation * 2.5 + 0.15);
    const float nightEV = baseEV - 4.5;
  return lerp(nightEV, baseEV + 1.2, dayFactor);
}

#endif // WE_SKY_ATMOSPHERE_HLSLI
