#pragma once

#include "EnvironmentDirectionalLight.h"
#include "EnvironmentHeightFog.h"
#include "EnvironmentSkyAtmosphere.h"
#include "EnvironmentSkyLight.h"
#include "EnvironmentVolumetricClouds.h"
#include "Renderer/SceneRenderer.hpp"
#include <glm/glm.hpp>

namespace we::runtime::world::environment {

glm::vec3 TemperatureKelvinToRgb(int kelvin);
glm::vec3 EulerDegreesToLightDirection(const glm::vec3& rotationDegrees);

we::runtime::renderer::SceneEnvironmentUniform BuildSceneEnvironmentUniform(
    const EnvironmentDirectionalLight& sun,
    const EnvironmentSkyLight& skyLight,
    const EnvironmentSkyAtmosphere& atmosphere,
    const EnvironmentHeightFog& fog,
    const EnvironmentVolumetricClouds& clouds);

} // namespace we::runtime::world::environment
