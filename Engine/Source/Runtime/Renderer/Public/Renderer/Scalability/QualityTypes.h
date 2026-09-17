// ==============================================================================
// WindEffects — Renderer — QualityTypes
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Renderer/Export.h"

#include <cstdint>
#include <string_view>

namespace we::runtime::renderer {

/// Named configuration presets. Never select an RHI backend or renderer fork.
/// HighEnd is the production target; Balanced/Low are dormant scalability presets.
enum class RenderingProfileId : uint8_t {
    HighEnd = 0,
    Balanced = 1,
    Low = 2,
    Custom = 255
};

/// Per-feature quality tier. Disabled means the feature is off (not merely low).
enum class QualityLevel : uint8_t {
    Disabled = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Ultra = 4,
    Epic = 5
};

/// How a profile feature depends on an RHI capability.
enum class FeatureRequirement : uint8_t {
    Off = 0,
    Optional = 1,
    Preferred = 2,
    Required = 3
};

/// Impact of applying a new resolved configuration at runtime.
enum class ScalabilityUpdateFlags : uint32_t {
    None = 0,
    UpdateSettings = 1u << 0,
    RecreateResources = 1u << 1,
    RebuildRenderGraph = 1u << 2,
    RecreatePipelines = 1u << 3,
    RestartRequired = 1u << 4
};

[[nodiscard]] constexpr ScalabilityUpdateFlags operator|(
    ScalabilityUpdateFlags a, ScalabilityUpdateFlags b) noexcept {
    return static_cast<ScalabilityUpdateFlags>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

[[nodiscard]] constexpr ScalabilityUpdateFlags operator&(
    ScalabilityUpdateFlags a, ScalabilityUpdateFlags b) noexcept {
    return static_cast<ScalabilityUpdateFlags>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

[[nodiscard]] constexpr bool HasFlag(
    ScalabilityUpdateFlags flags, ScalabilityUpdateFlags bit) noexcept {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(bit)) != 0;
}

[[nodiscard]] RENDERER_API const char* ToString(RenderingProfileId id) noexcept;
[[nodiscard]] RENDERER_API const char* ToString(QualityLevel level) noexcept;
[[nodiscard]] RENDERER_API std::string_view ProfileFileStem(RenderingProfileId id) noexcept;
[[nodiscard]] RENDERER_API RenderingProfileId ParseRenderingProfileId(std::string_view name) noexcept;
[[nodiscard]] RENDERER_API QualityLevel ClampQuality(QualityLevel level, QualityLevel maxLevel) noexcept;

} // namespace we::runtime::renderer
