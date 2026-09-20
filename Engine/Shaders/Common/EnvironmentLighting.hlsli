#ifndef WE_ENVIRONMENT_LIGHTING_HLSLI
#define WE_ENVIRONMENT_LIGHTING_HLSLI

// Shared atmosphere → environment lighting contracts.
// Visible sky uses AtmosphereIntegrator in ProceduralSky (full single-scatter).
// SkyLight / surface ambient use irradiance helpers here — same coeffs, one model.
// Not a painted PNG / fixed blue ambient.

#include "EnvironmentBuffer.hlsli"
#include "Math.hlsli"

#ifndef WE_PI
#define WE_PI 3.14159265358979323846
#endif

struct WE_EnvLightSample
{
    float3 skyRadiance;
    float3 skyIrradiance;
    float3 sunRadiance;
    float3 sunTransmittance;
};

float3 WE_EnvSunTransmittance(float3 toSun, float3 rayleigh, float mie, float3 ozone)
{
    const float cosZenith = saturate(toSun.y);
    const float airMass = 1.0 / max(cosZenith + 0.15, 0.08);
    const float3 optical =
        (rayleigh * 8.0 + mie * 1.2 + ozone * 0.5) * airMass * 0.35;
    return exp(-max(optical, 0.0));
}

float WE_EnvRayleighPhase(float cosTheta)
{
    return (3.0 / (16.0 * WE_PI)) * (1.0 + cosTheta * cosTheta);
}

float WE_EnvMiePhase(float cosTheta, float g)
{
    const float g2 = g * g;
    const float num = (1.0 - g2);
    const float denom = pow(max(1.0 + g2 - 2.0 * g * cosTheta, 1e-4), 1.5);
    return (3.0 / (8.0 * WE_PI)) * ((1.0 + g2) * num / denom);
}

float3 WE_EnvHorizonScatter(float3 toSun, float3 rayleigh)
{
    const float elev = saturate(toSun.y);
    const float rSum = max(dot(rayleigh, float3(1, 1, 1)), 1e-6);
    const float3 rN = rayleigh / rSum;
    float3 h = float3(0.50 + rN.x * 0.18, 0.60 + rN.y * 0.16, 0.75 + rN.z * 0.12);
    return h * (0.75 + 0.35 * elev);
}

/// Lightweight scattering sky for shared/fallback paths.
float3 WE_EvalSkyRadiance(
    float3 viewDir,
    float3 sunTravelDir,
    float3 sunCol,
    float sunIntensity,
    float sunAngularRadius,
    float enableDisk,
    float3 rayleigh,
    float mie,
    float3 ozone,
    float mieG)
{
    viewDir = normalize(viewDir);
    const float3 toSun = normalize(-sunTravelDir);
    const float elev = saturate(toSun.y);
    const float cosV = viewDir.y;
    const float cosSun = saturate(dot(viewDir, toSun));

    const float3 sunT = WE_EnvSunTransmittance(toSun, rayleigh, mie, ozone);
    const float phaseR = WE_EnvRayleighPhase(dot(viewDir, toSun));
    const float phaseM = WE_EnvMiePhase(dot(viewDir, toSun), clamp(mieG, 0.0, 0.95));

    const float airMass = 1.0 / max(abs(cosV) + 0.15, 0.08);
    const float3 optical = (rayleigh * 8.0 + mie * 1.2 + ozone * 0.35) * airMass * 0.22;
    const float3 viewT = exp(-max(optical, 0.0));

    const float heightCue = exp(-saturate(1.0 - cosV) * 1.5);
    const float3 sunRad = max(sunCol, 0.0) * max(sunIntensity, 0.0);
    float3 rayleighL = sunRad * sunT * viewT * rayleigh * phaseR * (2.5 + 4.0 * elev) * heightCue;
    float3 mieL = sunRad * sunT * viewT * mie * phaseM * (1.2 + 2.5 * elev);

    const float ozoneW = saturate(dot(ozone, float3(1, 1, 1)) * 80.0) * saturate(cosV);
    rayleighL *= lerp(float3(1, 1, 1), float3(0.92, 0.88, 1.0), ozoneW);

    float3 sky = rayleighL + mieL;
    sky += rayleigh * sunRad * sunT * (0.15 + 0.35 * elev) * (0.4 + 0.6 * saturate(cosV));

    const float haze = pow(saturate(1.0 - abs(cosV)), 5.0);
    sky += (WE_EnvHorizonScatter(toSun, rayleigh) * 0.35 + float3(0.04, 0.05, 0.06))
        * haze * (0.4 + mie * 50.0) * max(elev, 0.05);

    if (cosV < 0.0)
    {
        const float ground = saturate(-cosV);
        sky = lerp(sky, float3(0.10, 0.11, 0.12) * (0.3 + 0.7 * elev), smoothstep(0.0, 0.4, ground));
    }

    if (enableDisk > 0.5 && elev > -0.02)
    {
        const float ang = max(sunAngularRadius, 0.004675);
        const float disc = smoothstep(cos(ang * 1.5), cos(ang * 0.85), cosSun);
        sky += sunRad * sunT * disc * 8.0 * saturate(elev * 4.0 + 0.2);
    }

    return max(sky, 0.0);
}

float3 WE_EvalSkyIrradianceUpper(
    float3 sunTravelDir,
    float3 sunCol,
    float sunIntensity,
    float3 rayleigh,
    float mie,
    float3 ozone,
    float multiScatter,
    float skyLightIntensity)
{
    const float3 toSun = normalize(-sunTravelDir);
    const float elev = saturate(toSun.y);
    const float rSum = max(dot(rayleigh, float3(1, 1, 1)), 1e-6);
    const float3 rN = rayleigh / rSum;

    float3 zenith = float3(0.05 + rN.x * 0.12, 0.18 + rN.y * 0.32, 0.42 + rN.z * 0.55);
    float3 horizon = float3(0.45 + rN.x * 0.18, 0.55 + rN.y * 0.16, 0.70 + rN.z * 0.14);
    zenith *= (0.55 + 0.70 * elev);
    horizon *= (0.70 + 0.40 * elev);

    float3 irr = lerp(horizon, zenith, 0.55) * (0.50 + 0.70 * elev);
    const float3 sunT = WE_EnvSunTransmittance(toSun, rayleigh, mie, ozone);
    irr += sunCol * sunT * (0.10 * elev * max(sunIntensity, 0.0));
    irr *= max(multiScatter, 0.25) * max(skyLightIntensity, 0.0);
    return max(irr, 0.0);
}

WE_EnvLightSample WE_EvalEnvironmentLighting(float3 viewDir)
{
    WE_EnvLightSample s;
    const float3 toSun = normalize(-sunDirection);
    s.sunTransmittance = WE_EnvSunTransmittance(
        toSun, atmosphereRayleigh, mieScattering, ozoneAbsorption);
    s.sunRadiance = max(sunColor, 0.0) * max(sunIntensity, 0.0) * s.sunTransmittance;
    // No solar disk in environment radiance — SkyLight is illumination only.
    // Visible disk is owned exclusively by ProceduralSky (angular WE_ComputeSunDisk).
    s.skyRadiance = WE_EvalSkyRadiance(
        viewDir, sunDirection, sunColor, sunIntensity,
        sunAngularRadius, 0.0,
        atmosphereRayleigh, mieScattering, ozoneAbsorption, mieAnisotropy);
    s.skyIrradiance = WE_EvalSkyIrradianceUpper(
        sunDirection, sunColor, sunIntensity,
        atmosphereRayleigh, mieScattering, ozoneAbsorption,
        multiScatterStrength, skyLightIntensity);
    return s;
}

#endif // WE_ENVIRONMENT_LIGHTING_HLSLI
