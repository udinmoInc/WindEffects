// ==============================================================================
// WindEffects — Renderer — IIndirectLightingProvider
// Abstraction for future GI (probes / DDGI / RT). Surfaces + volumetrics consume this.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// ==============================================================================
#pragma once

#include "Core/Math/Types.h"
#include "Renderer/Export.h"

#pragma warning(push)
#pragma warning(disable : 4251)

namespace we::runtime::renderer {

struct IndirectLightingSample {
    we::math::Vec3 diffuseIrradiance{0.0f, 0.0f, 0.0f};
    we::math::Vec3 specularRadiance{0.0f, 0.0f, 0.0f};
};

class RENDERER_API IIndirectLightingProvider {
public:
    virtual ~IIndirectLightingProvider() = default;

    [[nodiscard]] virtual bool IsEnabled() const = 0;
    [[nodiscard]] virtual const char* GetName() const = 0;

    [[nodiscard]] virtual IndirectLightingSample Sample(
        const we::math::Vec3& worldPos,
        const we::math::Vec3& normal,
        const we::math::Vec3& viewDir) const = 0;
};

class RENDERER_API NullIndirectLightingProvider final : public IIndirectLightingProvider {
public:
    [[nodiscard]] bool IsEnabled() const override { return false; }
    [[nodiscard]] const char* GetName() const override { return "NullIndirect"; }

    [[nodiscard]] IndirectLightingSample Sample(
        const we::math::Vec3&,
        const we::math::Vec3&,
        const we::math::Vec3&) const override {
        return {};
    }
};

} // namespace we::runtime::renderer

#pragma warning(pop)
