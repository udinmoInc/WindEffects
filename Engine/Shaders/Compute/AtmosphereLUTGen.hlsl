#include "../Common/EnvironmentBuffer.hlsli"
#include "AtmosphereIntegrator.hlsli"

cbuffer LUTGenPass : register(b1)
{
    int passIndex;
    int3 padding;
};

RWTexture2D<float4> TransmittanceLUT     : register(u0);
RWTexture2D<float4> MultiScatteringLUT   : register(u1);
RWTexture2D<float4> SkyViewLUT         : register(u2);
RWTexture2D<float4> AerialPerspectiveLUT : register(u3);

WE_AtmosphereParams WE_ParamsFromEnvironment()
{
    const float3 sunLinear = max(sunColor, float3(0.0, 0.0, 0.0));
    return WE_BuildAtmosphereParams(
        max(atmosphereRayleigh, float3(1e-6, 1e-6, 1e-6)),
        max(mieScattering, 1e-6),
        max(ozoneAbsorption, float3(0.0, 0.0, 0.0)),
        mieAnisotropy,
        planetRadius,
        atmosphereHeight,
        multiScatterStrength,
        eyeAltitude,
        sunLinear,
        sunIntensity,
        max(sunAngularRadius, WE_SUN_ANGULAR_RADIUS));
}

[numthreads(8, 8, 1)]
void CSMain(uint3 dtid : SV_DispatchThreadID)
{
    WE_AtmosphereParams params = WE_ParamsFromEnvironment();
    const float3 sunDir = normalize(-sunDirection);

    if (passIndex == 0)
    {
        if (dtid.x >= WE_TRANSMITTANCE_LUT_WIDTH || dtid.y >= WE_TRANSMITTANCE_LUT_HEIGHT)
            return;

        const float hNorm = (float(dtid.x) + 0.5) / float(WE_TRANSMITTANCE_LUT_WIDTH);
        const float muNorm = (float(dtid.y) + 0.5) / float(WE_TRANSMITTANCE_LUT_HEIGHT);
        const float heightKm = hNorm * (params.atmosphereRadius - params.planetRadius);
        const float cosZenith = muNorm * 2.0 - 1.0;
        const float3 origin = float3(0.0, params.planetRadius + heightKm, 0.0);
        const float3 viewDir = float3(0.0, cosZenith, sqrt(max(1.0 - cosZenith * cosZenith, 0.0)));

        float3 rd, md, od;
        const float dist = params.atmosphereRadius - length(origin);
        const float3 trans = WE_IntegrateOpticalDepth(origin, viewDir, max(dist, 0.001), params, rd, md, od);
        TransmittanceLUT[dtid.xy] = float4(trans, 1.0);
        return;
    }

    if (passIndex == 1)
    {
        if (dtid.x >= WE_MULTISCATTER_LUT_SIZE || dtid.y >= WE_MULTISCATTER_LUT_SIZE)
            return;

        const float cosSun = (float(dtid.x) + 0.5) / float(WE_MULTISCATTER_LUT_SIZE) * 2.0 - 1.0;
        const float hNorm = (float(dtid.y) + 0.5) / float(WE_MULTISCATTER_LUT_SIZE);
        const float heightKm = hNorm * (params.atmosphereRadius - params.planetRadius);
        const float3 origin = float3(0.0, params.planetRadius + heightKm, 0.0);

        float3 rd, md, od;
        const float dist = params.atmosphereRadius - length(origin);
        WE_IntegrateOpticalDepth(origin, float3(0.0, 1.0, 0.0), max(dist, 0.001), params, rd, md, od);
        const float3 multi = (1.0 - exp(-(rd + md) * 0.15)) * params.rayleighCoeff * (0.5 + cosSun * 0.5);
        MultiScatteringLUT[dtid.xy] = float4(multi, 1.0);
        return;
    }

    if (passIndex == 2)
    {
        if (dtid.x >= WE_SKYVIEW_LUT_WIDTH || dtid.y >= WE_SKYVIEW_LUT_HEIGHT)
            return;

        const float azimuth = (float(dtid.x) + 0.5) / float(WE_SKYVIEW_LUT_WIDTH);
        const float viewZenith = (float(dtid.y) + 0.5) / float(WE_SKYVIEW_LUT_HEIGHT);
        const float theta = viewZenith * WE_PI;
        const float phi = azimuth * 2.0 * WE_PI;
        const float3 viewDir = float3(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));

        const float3 planetCenter = float3(0.0, -params.planetRadius, 0.0);
        const float3 origin = float3(0.0, params.eyeAltitude, 0.0) - planetCenter;
        float3 transmittance;
        float3 sky = WE_IntegrateInscattering(viewDir, sunDir, origin, params, transmittance);
        SkyViewLUT[dtid.xy] = float4(max(sky, 0.0), 1.0);
        return;
    }

    if (passIndex == 3)
    {
        if (dtid.x >= WE_AERIAL_LUT_WIDTH || dtid.y >= WE_AERIAL_LUT_HEIGHT)
            return;

        const float distKm = (float(dtid.x) + 0.5) / float(WE_AERIAL_LUT_WIDTH) * 64.0;
        const float heightKm = (float(dtid.y) + 0.5) / float(WE_AERIAL_LUT_HEIGHT)
            * (params.atmosphereRadius - params.planetRadius);
        const float3 viewDir = normalize(float3(0.3, 0.15, 1.0));
        const float3 surfacePos = float3(0.0, heightKm * 1000.0, distKm * 1000.0);
        const float3 camPos = float3(0.0, params.eyeAltitude * 1000.0, 0.0);
        const float3 marchDir = normalize(surfacePos - camPos);
        const float3 origin = WE_GetAtmosphereOrigin(camPos, worldOrigin, params.planetRadius);
        float3 transmittance;
        float3 inscatter = WE_IntegrateInscattering(marchDir, sunDir, origin, params, transmittance);
        const float fade = 1.0 - exp(-distKm * 0.06);
        AerialPerspectiveLUT[dtid.xy] = float4(inscatter * fade, 1.0);
    }
}
