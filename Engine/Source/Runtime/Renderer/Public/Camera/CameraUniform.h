#pragma once

#include "Renderer/Export.h"
#include <cstdint>
#if WE_HAS_GLM
#include "Core/Math/Types.h"
#endif

namespace we::runtime::renderer {

struct CameraUniform {
#if WE_HAS_GLM
    we::math::Mat4 view{};
    we::math::Mat4 proj{};
    we::math::Mat4 invViewProj{};
    we::math::Vec3 position{};
#else
    float view[16]{};
    float proj[16]{};
    float invViewProj[16]{};
    float position[3]{};
#endif
    float padding = 0.0f;
};

} // namespace we::runtime::renderer
