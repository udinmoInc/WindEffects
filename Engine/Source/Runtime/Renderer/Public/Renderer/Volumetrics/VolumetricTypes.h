// ==============================================================================
// WindEffects — Renderer — VolumetricTypes
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <cstdint>

namespace we::runtime::renderer {

enum class VolumetricProviderType : uint32_t {
    Cloud = 0,
    HeightFog = 1,
    LocalFog = 2,
    Smoke = 3,
    Dust = 4,
    Steam = 5,
    Gas = 6,
    Custom = 7,
    Atmosphere = 8,
};

/// Bitmask of active volumetric providers for the shared frame uniform.
struct VolumetricProviderMask {
    uint32_t bits = 0;

    [[nodiscard]] static constexpr uint32_t Bit(VolumetricProviderType type) {
        return 1u << static_cast<uint32_t>(type);
    }

    void Clear() { bits = 0; }

    void Set(VolumetricProviderType type) { bits |= Bit(type); }

    void Unset(VolumetricProviderType type) { bits &= ~Bit(type); }

    [[nodiscard]] bool Has(VolumetricProviderType type) const {
        return (bits & Bit(type)) != 0;
    }

    VolumetricProviderMask& operator|=(VolumetricProviderType type) {
        Set(type);
        return *this;
    }
};

} // namespace we::runtime::renderer
