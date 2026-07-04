#ifndef WE_SKY_ATMOSPHERE_HLSLI
#define WE_SKY_ATMOSPHERE_HLSLI

#include "../Common/Color.hlsli"
#include "AtmosphereLUT.hlsli"

// Camera-centered procedural atmosphere — no textures, gradients, or hardcoded sky colors.
// Rayleigh + Mie + ozone absorption + multi-scattering approximation + aerial perspective.

static const float WE_RAYLEIGH_SCALE_KM      = 8.0;
static const float WE_MIE_SCALE_KM           = 1.2;
static const float WE_OZONE_PEAK_ALT_KM      = 25.0;
static const float WE_OZONE_WIDTH_KM         = 8.0;
static const float WE_SUN_ANGULAR_RADIUS     = 0.004675;
static const int   WE_ATMOSPHERE_STEPS       = 24;
static const int   WE_SUN_TRANSMITTANCE_STEPS = 10;

struct WE_AtmosphereParams
{
    float3 rayleighCoeff;
    float  mieCoeff;
    float3 ozoneCoeff;
    float  mieAnisotropy;
    float  planetRadius;
    float  atmosphereRadius;
    float  multiScatterStrength;
    float  eyeAltitude;
    float3 sunColor;
    float  sunIntensity;
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

float WE_AtmosphereDensity(float heightKm, float scaleHeightKm)
{
    return exp(-max(heightKm, 0.0) / max(scaleHeightKm, 1e-3));
}

float WE_OzoneDensity(float heightKm)
{
    const float x = (heightKm - WE_OZONE_PEAK_ALT_KM) / WE_OZONE_WIDTH_KM;
    return exp(-x * x);
}

bool WE_IntersectSphere(float3 origin, float3 dir, float radius, out float t0, out float t1)
{
    t0 = 0.0;
    t1 = 0.0;
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

// Planet center is below the camera so the atmosphere shell is infinite relative to camera motion.
float3 WE_GetPlanetCenter(float3 cameraPos, float3 worldOrigin, float planetRadius)
{
    const float3 relCam = cameraPos - worldOrigin;
    return float3(relCam.x, relCam.y - planetRadius, relCam.z);
}

float3 WE_IntegrateSunTransmittance(
    float3 samplePosRel,
    float3 sunDir,
    WE_AtmosphereParams params)
{
    float3 opticalDepthRayleigh = float3(0.0, 0.0, 0.0);
    float3 opticalDepthMie = float3(0.0, 0.0, 0.0);
    float3 opticalDepthOzone = float3(0.0, 0.0, 0.0);

    const float stepSize = (params.atmosphereRadius - params.planetRadius) / float(WE_SUN_TRANSMITTANCE_STEPS);
    float3 marchPos = samplePosRel;

    [loop]
    for (int i = 0; i < WE_SUN_TRANSMITTANCE_STEPS; ++i)
    {
        const float heightKm = max(length(marchPos) - params.planetRadius, 0.0);
        const float rayleighDensity = WE_AtmosphereDensity(heightKm, WE_RAYLEIGH_SCALE_KM);
        const float mieDensity = WE_AtmosphereDensity(heightKm, WE_MIE_SCALE_KM);
        const float ozoneDensity = WE_OzoneDensity(heightKm);

        opticalDepthRayleigh += params.rayleighCoeff * rayleighDensity * stepSize;
        opticalDepthMie += params.mieCoeff * mieDensity * stepSize;
        opticalDepthOzone += params.ozoneCoeff * ozoneDensity * stepSize;

        marchPos += sunDir * stepSize;
    }

    return exp(-(opticalDepthRayleigh + opticalDepthMie + opticalDepthOzone));
}

float3 WE_ComputeAtmosphereScattering(
    float3 viewDir,
    float3 sunDir,
    float3 cameraPos,
    float3 worldOrigin,
    WE_AtmosphereParams params)
{
    viewDir = normalize(viewDir);
    sunDir = normalize(sunDir);

    const float3 planetCenter = WE_GetPlanetCenter(cameraPos, worldOrigin, params.planetRadius);
    const float3 rayOrigin = (cameraPos - worldOrigin) - planetCenter;
    const float3 origin = rayOrigin + normalize(rayOrigin + float3(0.0, params.eyeAltitude + 0.001, 0.0)) * 0.001;

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
    float3 opticalDepthOzone = float3(0.0, 0.0, 0.0);

    const float cosTheta = dot(viewDir, sunDir);
    const float phaseRayleigh = WE_RayleighPhase(cosTheta);
    const float phaseMie = WE_MiePhase(cosTheta, params.mieAnisotropy);

    [loop]
    for (int i = 0; i < WE_ATMOSPHERE_STEPS; ++i)
    {
        const float t = start + (float(i) + 0.5) * stepSize;
        const float3 samplePos = origin + viewDir * t;
        const float heightKm = max(length(samplePos) - params.planetRadius, 0.0);

        const float rayleighDensity = WE_AtmosphereDensity(heightKm, WE_RAYLEIGH_SCALE_KM);
        const float mieDensity = WE_AtmosphereDensity(heightKm, WE_MIE_SCALE_KM);
        const float ozoneDensity = WE_OzoneDensity(heightKm);

        opticalDepthRayleigh += params.rayleighCoeff * rayleighDensity * stepSize;
        opticalDepthMie += params.mieCoeff * mieDensity * stepSize;
        opticalDepthOzone += params.ozoneCoeff * ozoneDensity * stepSize;

        const float3 sunTransmittance = WE_IntegrateSunTransmittance(samplePos, sunDir, params);
        const float3 tau = opticalDepthRayleigh + opticalDepthMie + opticalDepthOzone;
        const float3 attenuation = exp(-tau) * sunTransmittance;

        sumRayleigh += attenuation * rayleighDensity * stepSize;
        sumMie += attenuation * mieDensity * stepSize;
    }

    // Multi-scattering approximation — isotropic ambient in-scatter from optically thick regions.
    const float3 opticalDepthTotal = opticalDepthRayleigh + opticalDepthMie;
    const float3 multiScatter = (1.0 - exp(-opticalDepthTotal * 0.15)) * params.rayleighCoeff
        * params.multiScatterStrength;

    const float3 sunRadiance = params.sunColor * params.sunIntensity;
    return sunRadiance * (
        sumRayleigh * phaseRayleigh * params.rayleighCoeff
        + sumMie * phaseMie * params.mieCoeff
        + multiScatter);
}

float3 WE_ComputeSunDisk(float3 viewDir, float3 sunDir, float intensity, float3 sunColor)
{
    const float cosAngle = dot(normalize(viewDir), normalize(sunDir));
    const float cosRadius = cos(WE_SUN_ANGULAR_RADIUS);
    const float disk = smoothstep(cosRadius, cosRadius + 0.0003, cosAngle);
    const float glow = pow(saturate(dot(viewDir, sunDir)), 128.0) * intensity * 0.12;
    return sunColor * intensity * (disk * 22.0 + glow);
}

WE_AtmosphereParams WE_BuildAtmosphereParams(
    float3 rayleighCoeff,
    float mieCoeff,
    float3 ozoneCoeff,
    float mieAnisotropy,
    float planetRadius,
    float atmosphereHeight,
    float multiScatterStrength,
    float eyeAltitude,
    float3 sunColor,
    float sunIntensity)
{
    WE_AtmosphereParams p;
    p.rayleighCoeff = max(rayleighCoeff, float3(1e-6, 1e-6, 1e-6));
    p.mieCoeff = max(mieCoeff, 1e-6);
    p.ozoneCoeff = max(ozoneCoeff, float3(0.0, 0.0, 0.0));
    p.mieAnisotropy = mieAnisotropy;
    p.planetRadius = max(planetRadius, 1.0);
    p.atmosphereRadius = p.planetRadius + max(atmosphereHeight, 0.1);
    p.multiScatterStrength = max(multiScatterStrength, 0.0);
    p.eyeAltitude = max(eyeAltitude, 0.001);
    p.sunColor = max(sunColor, float3(0.0, 0.0, 0.0));
    p.sunIntensity = max(sunIntensity, 0.0);
    return p;
}

float3 WE_SampleSkyAtmosphere(
    float3 viewDir,
    float3 lightTravelDir,
    float3 cameraPos,
    float3 worldOrigin,
    float3 sunColor,
    float sunIntensity,
    float3 rayleighCoeff,
    float mieCoeff,
    float3 ozoneCoeff,
    float mieAnisotropy,
    float planetRadius,
    float atmosphereHeight,
    float multiScatterStrength,
    float eyeAltitude)
{
    const float3 sunDir = normalize(-lightTravelDir);
    const float3 sunLinear = WE_sRGBToLinear(saturate(sunColor));
    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleighCoeff, mieCoeff, ozoneCoeff, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        sunLinear, sunIntensity, WE_SUN_ANGULAR_RADIUS);

    const float3 planetCenter = WE_GetPlanetCenter(cameraPos, worldOrigin, params.planetRadius);
    const float3 origin = (cameraPos - worldOrigin) - planetCenter;
    float3 transmittance;
    float3 sky = WE_IntegrateInscattering(viewDir, sunDir, origin, params, transmittance);
    sky += WE_ComputeSunDisk(viewDir, sunDir, sunIntensity, sunLinear, params.sunAngularRadius);
    return max(sky, 0.0);
}

float3 WE_SampleSkyLightHemisphere(
    float3 lightTravelDir,
    float3 cameraPos,
    float3 worldOrigin,
    float3 sunColor,
    float sunIntensity,
    float3 rayleighCoeff,
    float mieCoeff,
    float3 ozoneCoeff,
    float mieAnisotropy,
    float planetRadius,
    float atmosphereHeight,
    float multiScatterStrength,
    float eyeAltitude,
    bool upperHemisphere)
{
    const float3 basis = upperHemisphere ? float3(0.0, 1.0, 0.0) : float3(0.0, -1.0, 0.0);
    float3 sum = float3(0.0, 0.0, 0.0);
    const int samples = 4;
    [unroll]
    for (int i = 0; i < samples; ++i)
    {
        const float angle = (float(i) / float(samples)) * WE_PI * 2.0;
        const float3 offset = float3(cos(angle), 0.0, sin(angle)) * 0.35;
        const float3 dir = normalize(basis + offset);
        sum += WE_SampleSkyAtmosphere(
            dir, lightTravelDir, cameraPos, worldOrigin,
            sunColor, sunIntensity * 0.25,
            rayleighCoeff, mieCoeff, ozoneCoeff, mieAnisotropy,
            planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude);
    }
    return sum / float(samples);
}

#endif // WE_SKY_ATMOSPHERE_HLSLI
