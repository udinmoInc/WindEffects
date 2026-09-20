// ==============================================================================
// WindEffects — Renderer — EnvironmentLightingEvaluator
// ==============================================================================
#include "Lighting/EnvironmentLightingEvaluator.h"

#include <algorithm>
#include <cmath>

namespace we::runtime::renderer {
namespace {

float Length3(const we::math::Vec3& v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

we::math::Vec3 Normalize3(const we::math::Vec3& v, const we::math::Vec3& fallback) {
    const float len = Length3(v);
    if (len < 1.0e-8f) {
        return fallback;
    }
    const float inv = 1.0f / len;
    return {v.x * inv, v.y * inv, v.z * inv};
}

we::math::Vec3 ClampPos(const we::math::Vec3& v) {
    return {
        std::max(v.x, 0.0f),
        std::max(v.y, 0.0f),
        std::max(v.z, 0.0f)};
}

we::math::Vec3 Mul3(const we::math::Vec3& a, float s) {
    return {a.x * s, a.y * s, a.z * s};
}

we::math::Vec3 Mul3(const we::math::Vec3& a, const we::math::Vec3& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

we::math::Vec3 Add3(const we::math::Vec3& a, const we::math::Vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

we::math::Vec3 Lerp3(const we::math::Vec3& a, const we::math::Vec3& b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    return {
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t};
}

we::math::Vec3 ExpNeg(const we::math::Vec3& optical) {
    return {
        std::exp(-std::max(optical.x, 0.0f)),
        std::exp(-std::max(optical.y, 0.0f)),
        std::exp(-std::max(optical.z, 0.0f))};
}

} // namespace

we::math::Vec3 EnvironmentLightingEvaluator::EvaluateSunTransmittance(
    const we::math::Vec3& sunTravelDir,
    const we::math::Vec3& rayleigh,
    float mie,
    const we::math::Vec3& ozone) {
    // Sun travel dir points toward scene; elevation from -direction.y.
    const we::math::Vec3 toSun = Normalize3(
        {-sunTravelDir.x, -sunTravelDir.y, -sunTravelDir.z},
        {0.0f, 1.0f, 0.0f});
    const float cosZenith = std::clamp(toSun.y, 0.0f, 1.0f);
    // Approximate optical air mass.
    const float airMass = 1.0f / std::max(cosZenith + 0.15f, 0.08f);
    const we::math::Vec3 optical = {
        (rayleigh.x * 8.0f + mie * 1.2f + ozone.x * 0.5f) * airMass * 0.35f,
        (rayleigh.y * 8.0f + mie * 1.2f + ozone.y * 0.5f) * airMass * 0.35f,
        (rayleigh.z * 8.0f + mie * 1.2f + ozone.z * 0.5f) * airMass * 0.35f};
    return ExpNeg(optical);
}

void EnvironmentLightingEvaluator::EvaluateSkyRadianceColors(
    const we::math::Vec3& sunTravelDir,
    const we::math::Vec3& rayleigh,
    float sunElevationFactor,
    we::math::Vec3& outZenith,
    we::math::Vec3& outHorizon) {
    // Rayleigh coeffs are larger in blue — zenith must read blue, horizon pale haze.
    const float rSum = std::max(rayleigh.x + rayleigh.y + rayleigh.z, 1.0e-6f);
    const we::math::Vec3 rN = {rayleigh.x / rSum, rayleigh.y / rSum, rayleigh.z / rSum};
    outZenith = {
        0.08f + rN.x * 0.18f,
        0.22f + rN.y * 0.38f,
        0.48f + rN.z * 0.50f};
    outZenith = Mul3(outZenith, 0.70f + 0.55f * sunElevationFactor);

    outHorizon = {
        0.52f + rN.x * 0.20f,
        0.62f + rN.y * 0.18f,
        0.78f + rN.z * 0.12f};
    outHorizon = Mul3(outHorizon, 0.80f + 0.30f * sunElevationFactor);

    // Warm horizon when sun is low.
    const float twilight = std::clamp(1.0f - std::abs(sunElevationFactor) * 2.5f, 0.0f, 1.0f);
    if (twilight > 0.01f) {
        const we::math::Vec3 duskHorizon{0.95f, 0.55f, 0.32f};
        const we::math::Vec3 duskZenith{0.12f, 0.16f, 0.35f};
        outHorizon = Lerp3(outHorizon, duskHorizon, twilight * 0.55f);
        outZenith = Lerp3(outZenith, duskZenith, twilight * 0.40f);
    }

    // Night falloff.
    const we::math::Vec3 toSun = Normalize3(
        {-sunTravelDir.x, -sunTravelDir.y, -sunTravelDir.z},
        {0.0f, 1.0f, 0.0f});
    const float night = std::clamp(0.08f - toSun.y, 0.0f, 1.0f);
    outZenith = Lerp3(outZenith, {0.02f, 0.03f, 0.07f}, night);
    outHorizon = Lerp3(outHorizon, {0.03f, 0.04f, 0.08f}, night);
    outZenith = ClampPos(outZenith);
    outHorizon = ClampPos(outHorizon);
}

we::math::Vec3 EnvironmentLightingEvaluator::EvaluateSkyIrradianceUpper(
    const we::math::Vec3& sunTravelDir,
    const we::math::Vec3& sunRadiance,
    float sunIntensity,
    const we::math::Vec3& rayleigh,
    float mie,
    float multiScatter) {
    // Keep in sync with WE_EvalSkyIrradianceUpper (EnvironmentLighting.hlsli).
    const we::math::Vec3 toSun = Normalize3(
        {-sunTravelDir.x, -sunTravelDir.y, -sunTravelDir.z},
        {0.0f, 1.0f, 0.0f});
    const float elev = std::clamp(toSun.y, 0.0f, 1.0f);
    const float rSum = std::max(rayleigh.x + rayleigh.y + rayleigh.z, 1.0e-6f);
    const we::math::Vec3 rN = {rayleigh.x / rSum, rayleigh.y / rSum, rayleigh.z / rSum};

    we::math::Vec3 zenith = {
        0.05f + rN.x * 0.12f,
        0.18f + rN.y * 0.32f,
        0.42f + rN.z * 0.55f};
    we::math::Vec3 horizon = {
        0.45f + rN.x * 0.18f,
        0.55f + rN.y * 0.16f,
        0.70f + rN.z * 0.14f};
    zenith = Mul3(zenith, 0.55f + 0.70f * elev);
    horizon = Mul3(horizon, 0.70f + 0.40f * elev);

    we::math::Vec3 irr = Lerp3(horizon, zenith, 0.55f);
    irr = Mul3(irr, 0.50f + 0.70f * elev);

    const we::math::Vec3 ozone = {0.00065f, 0.00188f, 0.000085f};
    const we::math::Vec3 sunT = EvaluateSunTransmittance(sunTravelDir, rayleigh, mie, ozone);
    irr = Add3(irr, Mul3(Mul3(sunRadiance, sunT), 0.10f * elev * std::max(sunIntensity, 0.0f)));
    irr = Mul3(irr, std::max(multiScatter, 0.25f));
    return ClampPos(irr);
}

EnvironmentLightingContext EnvironmentLightingEvaluator::FromEnvironmentUniform(
    const SceneEnvironmentUniform& env) {
    EnvironmentLightingContext ctx{};
    ctx.sunDirection = env.sunDirection;
    ctx.sunRadiance = env.sunColor;
    ctx.sunIntensity = env.sunIntensity;
    ctx.sunAngularRadius = env.sunAngularRadius;
    ctx.rayleigh = env.atmosphereRayleigh;
    ctx.mie = env.mieScattering;
    ctx.ozone = env.ozoneAbsorption;
    ctx.mieAnisotropy = env.mieAnisotropy;
    ctx.planetRadiusKm = env.planetRadius;
    ctx.atmosphereHeightKm = env.atmosphereHeight;
    ctx.multiScatterStrength = env.multiScatterStrength;
    ctx.eyeAltitudeKm = env.eyeAltitude;
    ctx.skyIntensity = env.skyLightIntensity;

    const we::math::Vec3 toSun = Normalize3(
        {-env.sunDirection.x, -env.sunDirection.y, -env.sunDirection.z},
        {0.0f, 1.0f, 0.0f});
    const float elev = std::clamp(toSun.y, 0.0f, 1.0f);

    EvaluateSkyRadianceColors(
        env.sunDirection, env.atmosphereRayleigh, elev,
        ctx.skyRadianceZenith, ctx.skyRadianceHorizon);

    ctx.sunTransmittance = EvaluateSunTransmittance(
        env.sunDirection, env.atmosphereRayleigh, env.mieScattering, env.ozoneAbsorption);

    ctx.skyIrradianceUpper = EvaluateSkyIrradianceUpper(
        env.sunDirection,
        env.sunColor,
        env.sunIntensity,
        env.atmosphereRayleigh,
        env.mieScattering,
        env.multiScatterStrength);

    // Lower hemisphere: ground bounce tinted by horizon + weak sun.
    ctx.skyIrradianceLower = Lerp3(
        env.skyLightLowerColor,
        Mul3(ctx.skyRadianceHorizon, 0.35f),
        0.65f);
    ctx.skyIrradianceLower = ClampPos(Mul3(ctx.skyIrradianceLower, 0.55f + 0.35f * elev));

    // Scale by artist sky-light intensity (modulation, not a separate palette).
    ctx.skyIrradianceUpper = Mul3(ctx.skyIrradianceUpper, std::max(env.skyLightIntensity, 0.0f));
    ctx.skyIrradianceLower = Mul3(ctx.skyIrradianceLower, std::max(env.skyLightIntensity, 0.0f));
    return ctx;
}

void EnvironmentLightingEvaluator::ApplyToEnvironmentUniform(
    SceneEnvironmentUniform& env,
    const EnvironmentLightingContext& ctx) {
    env.skyAmbientColor = ctx.skyIrradianceUpper;
    env.skyLightLowerColor = ctx.skyIrradianceLower;
}

} // namespace we::runtime::renderer
