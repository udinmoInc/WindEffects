#ifndef WE_VOLUMETRIC_SCATTERING_HLSLI
#define WE_VOLUMETRIC_SCATTERING_HLSLI

// Shared phase / extinction / multi-scatter helpers (media-agnostic).

float WE_HenyeyGreenstein(float cosTheta, float g)
{
    const float g2 = g * g;
    const float denom = max(1.0 + g2 - 2.0 * g * cosTheta, 1e-4);
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(denom, 1.5));
}

float WE_DualLobeHG(float cosTheta, float gForward, float gBack, float blend)
{
    const float forward = WE_HenyeyGreenstein(cosTheta, gForward);
    const float back = WE_HenyeyGreenstein(cosTheta, gBack);
    return lerp(back, forward, saturate(blend));
}

float WE_BeerLaw(float opticalDepth, float absorption)
{
    return exp(-opticalDepth * max(absorption, 1e-5));
}

float WE_PowderEffect(float densityAlongLight, float cosTheta, float powderStrength)
{
    const float powder = 1.0 - exp(-densityAlongLight * 2.0);
    const float viewDependent = saturate((-cosTheta) * 0.5 + 0.5);
    const float apply = saturate(powderStrength) * lerp(0.35, 0.85, viewDependent);
    return lerp(1.0, powder, apply);
}

float WE_BeerPowderMultiScatter(float optical, float absorption, float cosTheta, float powderStrength)
{
    const float sigma = clamp(absorption, 0.045, 0.12);
    const float beer = WE_BeerLaw(optical, sigma);
    const float powder = WE_PowderEffect(optical, cosTheta, powderStrength);
    const float beerPowder = 2.0 * beer * powder;
    const float sunlitFloor = beer * 0.45;
    const float direct = max(beerPowder, sunlitFloor);

    const float ms1 = WE_BeerLaw(optical, sigma * 0.50);
    const float ms2 = WE_BeerLaw(optical, sigma * 0.25);
    const float ms3 = WE_BeerLaw(optical, sigma * 0.125);
    const float multiScatter = beer * 0.50 + ms1 * 0.28 + ms2 * 0.15 + ms3 * 0.07;

    return direct * 0.75 + multiScatter * (1.0 - beer) * 0.70;
}

float WE_SilverLining(float cosTheta, float intensity, float spread)
{
    const float lobe = pow(saturate(cosTheta), max(spread, 1.0));
    return 1.0 + saturate(intensity) * lobe * 0.55;
}

float WE_ViewTransmittance(float density, float stepLength, float extinctionCoeff)
{
    return exp(-density * stepLength * max(extinctionCoeff, 1e-5));
}

#endif // WE_VOLUMETRIC_SCATTERING_HLSLI
