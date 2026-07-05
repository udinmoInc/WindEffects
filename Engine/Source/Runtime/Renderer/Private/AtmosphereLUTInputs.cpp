#include "Renderer/AtmosphereLUTInputs.hpp"

#include <cmath>

namespace we::runtime::renderer {

namespace {

constexpr float kScalarEpsilon = 1e-4f;
constexpr float kDirectionDotEpsilon = 1e-5f;

bool NearlyEqual(float a, float b) {
    return std::fabs(a - b) <= kScalarEpsilon;
}

bool DirectionsNearlyEqual(const glm::vec3& a, const glm::vec3& b) {
    const float lenA = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
    const float lenB = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
    if (lenA <= 1e-6f || lenB <= 1e-6f) {
        return NearlyEqual(a.x, b.x) && NearlyEqual(a.y, b.y) && NearlyEqual(a.z, b.z);
    }
    const float dot = (a.x * b.x + a.y * b.y + a.z * b.z) / (lenA * lenB);
    return dot >= 1.0f - kDirectionDotEpsilon;
}

bool NearlyEqual(const glm::vec3& a, const glm::vec3& b) {
    return DirectionsNearlyEqual(a, b);
}

} // namespace

bool AtmosphereLUTInputsChanged(
    const SceneEnvironmentUniform& previous,
    const SceneEnvironmentUniform& next) {

    return !NearlyEqual(previous.sunDirection, next.sunDirection)
        || !NearlyEqual(previous.sunIntensity, next.sunIntensity)
        || !NearlyEqual(previous.sunColor, next.sunColor)
        || !NearlyEqual(previous.sunAngularRadius, next.sunAngularRadius)
        || !NearlyEqual(previous.atmosphereRayleigh, next.atmosphereRayleigh)
        || !NearlyEqual(previous.mieScattering, next.mieScattering)
        || !NearlyEqual(previous.ozoneAbsorption, next.ozoneAbsorption)
        || !NearlyEqual(previous.mieAnisotropy, next.mieAnisotropy)
        || !NearlyEqual(previous.planetRadius, next.planetRadius)
        || !NearlyEqual(previous.atmosphereHeight, next.atmosphereHeight)
        || !NearlyEqual(previous.multiScatterStrength, next.multiScatterStrength)
        || !NearlyEqual(previous.eyeAltitude, next.eyeAltitude)
        || !NearlyEqual(previous.worldOrigin, next.worldOrigin);
}

} // namespace we::runtime::renderer
