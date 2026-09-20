// ==============================================================================
// WindEffects — Renderer — IVolumetricProvider
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Camera/CameraUniform.h"
#include "Lighting/CloudUniform.h"
#include "Lighting/SceneEnvironmentUniform.h"
#include "Renderer/Scalability/RenderingSettings.h"
#include "Renderer/Volumetrics/LocalFogUniform.h"
#include "Renderer/Volumetrics/VolumetricTypes.h"

#include <cstdint>

namespace we::runtime::renderer {

/// Per-frame inputs shared by all volumetric providers. Pointers may be null.
struct VolumetricPrepareContext {
    const CameraUniform* camera = nullptr;
    const SceneEnvironmentUniform* environment = nullptr;
    const CloudUniform* cloud = nullptr;
    const LocalFogUniform* localFog = nullptr;
    const VolumetricQualitySettings* quality = nullptr;
    uint32_t frameIndex = 0;
    float deltaTime = 0.0f;
};

/// Small provider interface — no cloud-specific methods.
class IVolumetricProvider {
public:
    virtual ~IVolumetricProvider() = default;

    [[nodiscard]] virtual VolumetricProviderType GetType() const = 0;
    [[nodiscard]] virtual bool IsEnabled() const = 0;
    [[nodiscard]] virtual const char* GetName() const = 0;

    virtual void PrepareFrame(const VolumetricPrepareContext& ctx) = 0;
};

} // namespace we::runtime::renderer
