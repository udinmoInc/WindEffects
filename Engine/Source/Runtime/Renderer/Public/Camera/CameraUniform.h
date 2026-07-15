#pragma once

#include "Renderer/Export.h"
#include <cstdint>
#if WE_HAS_GLM
#include <glm/glm.hpp>
#endif

namespace we::runtime::renderer {

struct CameraUniform {
#if WE_HAS_GLM
    glm::mat4 view{};
    glm::mat4 proj{};
    glm::mat4 invViewProj{};
    glm::vec3 position{};
#else
    float view[16]{};
    float proj[16]{};
    float invViewProj[16]{};
    float position[3]{};
#endif
    float padding = 0.0f;
};

} // namespace we::runtime::renderer
