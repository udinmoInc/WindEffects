#ifndef WE_ATMOSPHERE_COMMON_HLSLI
#define WE_ATMOSPHERE_COMMON_HLSLI

#include "../Common/Math.hlsli"

static const float WE_RAYLEIGH_SCALE_KM      = 8.0;
static const float WE_MIE_SCALE_KM           = 1.2;
static const float WE_OZONE_PEAK_ALT_KM      = 25.0;
static const float WE_OZONE_WIDTH_KM         = 8.0;
static const float WE_SUN_ANGULAR_RADIUS     = 0.004675;
static const int   WE_ATMOSPHERE_STEPS       = 32;
static const int   WE_SUN_TRANSMITTANCE_STEPS = 16;
static const float WE_SKY_RADIANCE_SCALE     = 6.0;

static const int WE_TRANSMITTANCE_LUT_WIDTH  = 256;
static const int WE_TRANSMITTANCE_LUT_HEIGHT = 64;
static const int WE_MULTISCATTER_LUT_SIZE    = 32;
static const int WE_SKYVIEW_LUT_WIDTH        = 192;
static const int WE_SKYVIEW_LUT_HEIGHT       = 96;
static const int WE_AERIAL_LUT_WIDTH         = 32;
static const int WE_AERIAL_LUT_HEIGHT        = 32;

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
    float  sunAngularRadius;
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
    dir = normalize(dir);
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

// Forward distance along dir from origin to the atmosphere shell exit.
float WE_AtmosphereShellExitDistance(float3 origin, float3 dir, float radius)
{
    float t0 = 0.0;
    float t1 = 0.0;
    if (!WE_IntersectSphere(origin, dir, radius, t0, t1))
        return 0.0;
    if (t0 > 0.0)
        return t0;
    if (t1 > 0.0)
        return t1;
    return 0.0;
}

static const float WE_METERS_PER_KM = 1000.0;

float3 WE_WorldToAtmosphereKm(float3 worldPos, float3 worldOrigin)
{
    return (worldPos - worldOrigin) / WE_METERS_PER_KM;
}

// Planet center is at world origin. Returns observer position in atmosphere km space (planet-centered).
// Horizontal position is ignored so sky/fog stay world-stable while the editor camera moves in XZ.
float3 WE_GetAtmosphereOrigin(float3 cameraPos, float3 worldOrigin, float planetRadiusKm, float eyeAltitudeKm)
{
    const float3 relKm = WE_WorldToAtmosphereKm(cameraPos, worldOrigin);
    const float altitudeKm = max(max(relKm.y, 0.0), max(eyeAltitudeKm, 0.0));
    return float3(0.0, planetRadiusKm + altitudeKm, 0.0);
}

float3 WE_GetPlanetCenter(float3 cameraPos, float3 worldOrigin, float planetRadiusKm)
{
    const float3 relKm = WE_WorldToAtmosphereKm(cameraPos, worldOrigin);
    return float3(relKm.x, relKm.y - planetRadiusKm, relKm.z);
}

// UE5-style transmittance LUT mapping (distance along view ray vs shell height).
float2 WE_LutTransmittanceParamsToUv(float viewHeightKm, float viewZenithCosAngle, WE_AtmosphereParams params)
{
    const float bottomR = params.planetRadius;
    const float topR = params.atmosphereRadius;
    const float H = sqrt(max(topR * topR - bottomR * bottomR, 0.0));
    const float rho = sqrt(max(viewHeightKm * viewHeightKm - bottomR * bottomR, 0.0));

    const float discriminant = viewHeightKm * viewHeightKm * (viewZenithCosAngle * viewZenithCosAngle - 1.0)
        + topR * topR;
    const float d = max(0.0, (-viewHeightKm * viewZenithCosAngle + sqrt(max(discriminant, 0.0))));

    const float dMin = topR - viewHeightKm;
    const float dMax = rho + H;
    const float xMu = (d - dMin) / max(dMax - dMin, 1e-4);
    const float xR = rho / max(H, 1e-4);
    return float2(saturate(xMu), saturate(xR));
}

// Sun-relative azimuth (U) and normalized zenith angle (V) for SkyView LUT.
float2 WE_SkyViewLUTCoord(float3 viewDir, float3 sunDir, out float viewZenithAngle)
{
    const float3 vd = normalize(viewDir);
    const float3 sd = normalize(sunDir);
    viewZenithAngle = acos(saturate(vd.y)) / WE_PI;

    const float sunAzimuth = atan2(sd.z, sd.x);
    const float viewAzimuth = atan2(vd.z, vd.x);
    float azimuthOffset = (viewAzimuth - sunAzimuth) / (2.0 * WE_PI);
    azimuthOffset = azimuthOffset - floor(azimuthOffset);
    return float2(azimuthOffset, viewZenithAngle);
}

float3 WE_ViewDirFromSkyViewUV(float2 uv, float3 sunDir)
{
    const float viewZenith = uv.y * WE_PI;
    const float azimuthOffset = uv.x * 2.0 * WE_PI;
    const float sunAzimuth = atan2(sunDir.z, sunDir.x);
    const float azimuth = sunAzimuth + azimuthOffset;
    const float sinTheta = sin(viewZenith);
    return float3(sinTheta * cos(azimuth), cos(viewZenith), sinTheta * sin(azimuth));
}

float WE_ComputeSunDiskMask(float3 viewDir, float3 sunDir, float angularRadius)
{
    const float cosAngle = dot(normalize(viewDir), normalize(sunDir));
    const float cosRadius = cos(max(angularRadius, WE_SUN_ANGULAR_RADIUS));
    // edge0 < edge1 required; cosAngle rises toward 1 at the sun center.
    const float feather = 0.00015;
    return smoothstep(cosRadius - feather, cosRadius, cosAngle);
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
    float sunIntensity,
    float sunAngularRadius)
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
    p.sunAngularRadius = max(sunAngularRadius, 1e-5);
    return p;
}

#endif // WE_ATMOSPHERE_COMMON_HLSLI
