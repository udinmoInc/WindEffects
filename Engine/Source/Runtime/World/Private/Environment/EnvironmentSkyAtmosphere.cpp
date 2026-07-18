#include "Environment/EnvironmentSkyAtmosphere.h"

#include "Core/Math/GlmInterop.h"
namespace we::runtime::world::environment {

void EnvironmentSkyAtmosphere::ApplyDefaults() {
    RayleighScattering = 0.005802f;
    MieScattering = 0.003996f;
    MieAnisotropy = 0.76f;
    MultiScatterStrength = 1.0f;
    EyeAltitude = 0.001f;
    OzoneAbsorption = we::math::Vec3(0.00065f, 0.0018f, 0.00008f);
    AerialPerspectiveStartDepth = 0.1f;
    GroundAlbedo = we::math::Vec3(0.4f, 0.4f, 0.4f);
}

we::math::Vec3 EnvironmentSkyAtmosphere::GetRayleighColor() const {
    // Sea-level Rayleigh scattering coefficients (1/km), matching UE5 reference values.
    constexpr float kRed = 0.005802f;
    constexpr float kGreen = 0.013558f;
    constexpr float kBlue = 0.033100f;
    const float scale = RayleighScattering / kRed;
    return we::math::Vec3(kRed * scale, kGreen * scale, kBlue * scale);
}

we::math::Vec3 EnvironmentSkyAtmosphere::GetOzoneAbsorption() const {
    return OzoneAbsorption;
}

} // namespace we::runtime::world::environment
