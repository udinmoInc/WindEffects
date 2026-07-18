#pragma once

#include "Renderer/Export.h"
#include "Core/Math/Types.h"

#include <cstdint>

namespace we::runtime::renderer {

struct CameraUniform {
    we::math::Mat4 view{};
    we::math::Mat4 proj{};
    we::math::Mat4 invViewProj{};
    we::math::Vec3 position{};
    float padding = 0.0f;
};

} // namespace we::runtime::renderer
