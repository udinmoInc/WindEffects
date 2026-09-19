// ==============================================================================
// WindEffects — Renderer — LightingScene
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#pragma warning(push)
#pragma warning(disable : 4251)

#include "Lighting/LightingTypes.h"
#include "Renderer/Export.h"

#include <vector>

namespace we::runtime::renderer {

/// CPU-side lighting data for the current frame. No GPU resources live here.
struct RENDERER_API LightingScene {
    std::vector<DirectionalLight> directionalLights;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    EnvironmentLighting environment{};

    void Clear() {
        directionalLights.clear();
        pointLights.clear();
        spotLights.clear();
        environment = {};
    }

    [[nodiscard]] const DirectionalLight* PrimaryDirectional() const {
        for (const auto& light : directionalLights) {
            if (light.enabled) {
                return &light;
            }
        }
        return nullptr;
    }
};

} // namespace we::runtime::renderer

#pragma warning(pop)
