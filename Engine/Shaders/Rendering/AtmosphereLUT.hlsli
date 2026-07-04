#ifndef WE_ATMOSPHERE_LUT_HLSLI
#define WE_ATMOSPHERE_LUT_HLSLI

#include "AtmosphereIntegrator.hlsli"

// Transmittance LUT: X = height (0..atmosphere top), Y = cos(view zenith)
float2 WE_TransmittanceLUTCoord(float heightKm, float cosZenith, WE_AtmosphereParams params)
{
    const float hNorm = saturate(heightKm / max(params.atmosphereRadius - params.planetRadius, 1.0));
    const float muNorm = cosZenith * 0.5 + 0.5;
    return float2(hNorm, muNorm);
}

float WE_SampleTransmittanceLUT(
    Texture2D lut,
    SamplerState samp,
    float heightKm,
    float cosZenith,
    WE_AtmosphereParams params)
{
    const float2 uv = WE_TransmittanceLUTCoord(heightKm, cosZenith, params);
    return lut.SampleLevel(samp, uv, 0.0).r;
}

float3 WE_SampleMultiScatteringLUT(
    Texture2D lut,
    SamplerState samp,
    float cosSunZenith,
    float heightKm,
    WE_AtmosphereParams params)
{
    const float hNorm = saturate(heightKm / max(params.atmosphereRadius - params.planetRadius, 1.0));
    const float2 uv = float2(cosSunZenith * 0.5 + 0.5, hNorm);
    return lut.SampleLevel(samp, uv, 0.0).rgb * params.multiScatterStrength;
}

float3 WE_SampleSkyViewLUT(
    Texture2D lut,
    SamplerState samp,
    float3 viewDir,
    float3 sunDir)
{
    const float3 vd = normalize(viewDir);
    const float3 sd = normalize(sunDir);
    const float viewZenith = acos(saturate(vd.y)) / WE_PI;
    const float sunZenith = acos(saturate(sd.y)) / WE_PI;
    const float3 right = normalize(cross(float3(0.0, 1.0, 0.0), sd));
    const float3 up = cross(sd, right);
    const float azimuth = atan2(dot(vd, right), dot(vd, up)) / (2.0 * WE_PI) + 0.5;
    const float2 uv = float2(azimuth, lerp(viewZenith, sunZenith, 0.15));
    return max(lut.SampleLevel(samp, uv, 0.0).rgb, 0.0);
}

float3 WE_SampleAerialPerspectiveLUT(
    Texture2D lut,
    SamplerState samp,
    float distanceKm,
    float heightKm,
    WE_AtmosphereParams params)
{
    const float dNorm = saturate(distanceKm / 64.0);
    const float hNorm = saturate(heightKm / max(params.atmosphereRadius - params.planetRadius, 1.0));
    return lut.SampleLevel(samp, float2(dNorm, hNorm), 0.0).rgb;
}

float3 WE_SampleSkyAtmosphereLUT(
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
    float eyeAltitude,
    float sunAngularRadius,
    Texture2D transmittanceLUT,
    Texture2D multiScatterLUT,
    Texture2D skyViewLUT,
    Texture2D aerialLUT,
    SamplerState lutSampler)
{
    const float3 sunDir = normalize(-lightTravelDir);
    const float3 sunLinear = WE_sRGBToLinear(saturate(sunColor));

    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleighCoeff, mieCoeff, ozoneCoeff, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        sunLinear, sunIntensity, sunAngularRadius);

    const float3 planetCenter = WE_GetPlanetCenter(cameraPos, worldOrigin, params.planetRadius);
    const float3 origin = (cameraPos - worldOrigin) - planetCenter;
    const float heightKm = max(length(origin) - params.planetRadius, 0.0);

    float3 sky = WE_SampleSkyViewLUT(skyViewLUT, lutSampler, viewDir, sunDir);

    const float cosSunZenith = dot(sunDir, normalize(float3(0.0, 1.0, 0.0)));
    sky += WE_SampleMultiScatteringLUT(multiScatterLUT, lutSampler, cosSunZenith, heightKm, params);

    const float transmittance = WE_SampleTransmittanceLUT(transmittanceLUT, lutSampler, heightKm, viewDir.y, params);
    sky *= lerp(0.35, 1.0, transmittance);

    sky += WE_ComputeSunDisk(viewDir, sunDir, sunIntensity, sunLinear, params.sunAngularRadius);
    return max(sky, 0.0);
}

float3 WE_SampleAerialPerspective(
    float3 viewDir,
    float3 lightTravelDir,
    float3 cameraPos,
    float3 worldOrigin,
    float3 surfacePos,
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
    float sunAngularRadius,
    Texture2D aerialLUT,
    Texture2D skyViewLUT,
    SamplerState lutSampler)
{
    const float3 relCam = cameraPos - worldOrigin;
    const float3 relSurf = surfacePos - worldOrigin;
    const float distKm = length(relCam - relSurf) * 0.001;
    const float heightKm = max(relSurf.y * 0.001, 0.0);

    WE_AtmosphereParams params = WE_BuildAtmosphereParams(
        rayleighCoeff, mieCoeff, ozoneCoeff, mieAnisotropy,
        planetRadius, atmosphereHeight, multiScatterStrength, eyeAltitude,
        WE_sRGBToLinear(saturate(sunColor)), sunIntensity, sunAngularRadius);

    const float3 sunDir = normalize(-lightTravelDir);
    const float3 horizonSky = WE_SampleSkyViewLUT(skyViewLUT, lutSampler, normalize(viewDir), sunDir);
    const float3 aerial = WE_SampleAerialPerspectiveLUT(aerialLUT, lutSampler, distKm, heightKm, params);
    return lerp(horizonSky, aerial, saturate(distKm * 0.08));
}

float WE_ComputeExposureEV(float3 lightTravelDir, float baseEV, float compensation)
{
    const float sunElevation = normalize(-lightTravelDir).y;
    const float dayFactor = saturate(sunElevation * 2.5 + 0.1);
    const float twilightFactor = saturate(1.0 - abs(sunElevation) * 3.0);
    const float nightEV = baseEV - 5.0;
    const float dayEV = baseEV + 1.0;
    const float twilightEV = baseEV - 1.5;
    float ev = lerp(nightEV, dayEV, dayFactor);
    ev = lerp(ev, twilightEV, twilightFactor * (1.0 - dayFactor));
    return ev + compensation;
}

#endif // WE_ATMOSPHERE_LUT_HLSLI
