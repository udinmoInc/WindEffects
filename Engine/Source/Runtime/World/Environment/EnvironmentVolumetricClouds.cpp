#include "Environment/EnvironmentVolumetricClouds.h"

namespace we::runtime::world::environment {

void EnvironmentVolumetricClouds::ApplyDefaults() {
    Enabled = true;
    Coverage = 0.45f;
    Altitude = 5000.0f;
    Extinction = 0.35f;
    CloudColor = glm::vec3(0.95f, 0.96f, 0.98f);
}

} // namespace we::runtime::world::environment
